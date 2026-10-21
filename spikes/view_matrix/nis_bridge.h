// NIS bridge adapter for SingularityVR - research/nvidia-vr-upscaling
//
// Goal: keep the production d3d9.cpp integration to a handful of explicit
// calls (see the NisBridge_* surface below) so a future cherry-pick onto the
// gameplay runtime touches as few shared lines as possible. All staging,
// dispatch, timing and lifetime policy lives here, header-only so the
// single-TU x86 build (cl d3d9.cpp) needs no build-script changes.
//
// Intended production calls:
//   NisBridge_InitIfNeeded(...)      lazy, first stereo frame; embedded
//                                    bytecode first, HLSL fallback, else native
//   NisBridge_OutputEye(...)         swapchain eye size from recommended
//   NisBridge_ShouldEngage(...)      pure gate (Off/unready/mono -> native)
//   NisBridge_EnsureFrame(...)       persistent stage+output textures
//   NisBridge_ProcessStereo(...)     2 isolated half-copies + 2 per-eye dispatches
//   NisBridge_PollTiming(...)        harvest GPU timestamps without stalling
//   NisBridge_Shutdown(...)          release everything (device reset/exit)
//
// Ownership stays explicit: the caller owns the D3D11 device/context,
// the SBS source texture lifetime, and the swapchain copies. The bridge owns
// only its stage/output textures, shader objects, queries and stats.

#pragma once

#include "nis_upscale.h"
#include "nis_runtime.h"

struct NisBridge {
    NisRuntime rt;
    volatile LONG mode = 0;          // nis_upscale::Mode as LONG (0 Off, 1 NIS, 2 NISSharpen)
    float outputScale = 1.0f;        // swapchain multiplier (recommended-based)
    volatile LONG sharpnessPct = 20; // atomic UI->render setting, 0..100
    // gentle: the official curve hits USM strength ~1.1 at 0.35, which
    // reads as haloing through the lenses. Live-tuned from the panel.
    // NOTE: renderScale lives in d3d9.cpp as g_nisRenderScaleF (needed at
    // DllMain-time for the command-line hook, before any bridge exists) -
    // deliberately NOT duplicated here.
    ID3D11Texture2D* stageTex[2] = {};  // isolated eyes: filter cannot cross seam
    ID3D11ShaderResourceView* stageSrv[2] = {};
    UINT stageW = 0, stageH = 0;
    unsigned numStageCreates = 0;    // lifetime counter: must not move per frame
    // Accumulated stats (flushed on the existing per-second perf cadence).
    double msGpu = 0.0, msGpuPeak = 0.0;
    int gpuFrames = 0, lastTimingFrames = 0;
    double msStage = 0.0;
    int stageFrames = 0;
};

static float NisBridge_Sharpness(const NisBridge* b) {
    return nis_upscale::ClampSharpness((float)InterlockedCompareExchange(
        const_cast<volatile LONG*>(&b->sharpnessPct),0,0) * .01f);
}

// <dll-dir>\nis\NIS_Main.hlsl, beside this DLL (never CWD-dependent).
// Header-inline in the d3d9 TU, so the module handle is the mod DLL itself.
static void NisBridge_HlslPath(wchar_t* out, size_t cap) {
    HMODULE self = nullptr;
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            (LPCSTR)&NisBridge_HlslPath, &self)) { out[0] = 0; return; }
    wchar_t dir[MAX_PATH] = {};
    GetModuleFileNameW(self, dir, MAX_PATH);
    wchar_t* slash = wcsrchr(dir, L'\\');
    size_t head = slash ? (size_t)(slash - dir + 1) : 0;
    _snwprintf_s(out, cap, _TRUNCATE, L"%.*snis\\NIS_Main.hlsl", (int)head, dir);
}

// Swapchain eye size: recommended * outputScale, never the reduced render.
static void NisBridge_OutputEye(const NisBridge* b,
                                UINT recEyeW, UINT recEyeH,
                                UINT fbEyeW, UINT fbEyeH,
                                UINT* w, UINT* h) {
    // Sharpen-only means no resampling, no swapchain resolution change.
    if (InterlockedCompareExchange(const_cast<volatile LONG*>(&b->mode),0,0)
            == nis_upscale::kNISSharpen) { *w = fbEyeW; *h = fbEyeH; return; }
    nis_upscale::ComputeOutputEye(recEyeW, recEyeH, b->outputScale,
                                  fbEyeW, fbEyeH, w, h);
}

// Pure gate; Off/unready/failed/mono -> native path.
static bool NisBridge_ShouldEngage(const NisBridge* b, bool cropEyes, bool haveSrc) {
    return nis_upscale::ShouldEngageUpscale((int)b->mode, b->rt.ready,
        b->rt.failedOnce, cropEyes, haveSrc);
}

static bool NisBridge_InitIfNeeded(NisBridge* b, ID3D11Device* dev,
        const unsigned char* scalerBytes, unsigned int scalerSize,
        const unsigned char* sharpenBytes, unsigned int sharpenSize,
        const wchar_t* hlslFallbackPath, NisLogFn* logFn) {
    if (b->rt.ready || b->rt.failedOnce) return b->rt.ready;
    return NisInit(&b->rt, dev, scalerBytes, scalerSize,
                   sharpenBytes, sharpenSize, hlslFallbackPath, logFn);
}

static bool NisBridge_EnsureStage(NisBridge* b, ID3D11Device* dev,
                                  UINT sbsW, UINT sbsH) {
    if (b->stageTex[0] && b->stageSrv[0] && b->stageTex[1] && b->stageSrv[1] &&
        sbsW == b->stageW && sbsH == b->stageH) return true;
    for (int eye=0; eye<2; ++eye) {
        if (b->stageSrv[eye]) { b->stageSrv[eye]->Release(); b->stageSrv[eye] = nullptr; }
        if (b->stageTex[eye]) { b->stageTex[eye]->Release(); b->stageTex[eye] = nullptr; }
    }
    b->stageW = b->stageH = 0;
    if (!dev || sbsW < 2 || (sbsW % 2) || !sbsH) return false;
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = sbsW / 2; td.Height = sbsH; td.MipLevels = 1; td.ArraySize = 1;
    td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    for (int eye=0; eye<2; ++eye) {
        if (FAILED(dev->CreateTexture2D(&td, nullptr, &b->stageTex[eye]))) return false;
        if (FAILED(dev->CreateShaderResourceView(b->stageTex[eye], nullptr, &b->stageSrv[eye])))
            return false;
    }
    b->stageW = sbsW; b->stageH = sbsH;
    ++b->numStageCreates;
    return true;
}

static bool NisBridge_EnsureFrame(NisBridge* b, ID3D11Device* dev,
                                  UINT sbsW, UINT sbsH,
                                  UINT eyeOutW, UINT eyeOutH) {
    if (!NisBridge_EnsureStage(b, dev, sbsW, sbsH)) return false;
    return NisEnsureOutput(&b->rt, dev, sbsW / 2, sbsH, eyeOutW, eyeOutH);
}

// Eye stage copies + timestamped per-eye dispatches. Returns false when the caller
// must submit the native halves for this frame instead.
static bool NisBridge_ProcessStereo(NisBridge* b, ID3D11DeviceContext* ctx,
        ID3D11Texture2D* srcTex, UINT sbsW, UINT sbsH,
        UINT eyeOutW, UINT eyeOutH) {
    if (!b->rt.ready || !srcTex || !b->stageTex[0] || !b->stageSrv[0] ||
        !b->stageTex[1] || !b->stageSrv[1]) return false;
    LARGE_INTEGER t0{}; QueryPerformanceCounter(&t0);
    for (UINT eye=0; eye<2; ++eye) {
        D3D11_BOX box{eye*(sbsW/2),0,0,(eye+1)*(sbsW/2),sbsH,1};
        ctx->CopySubresourceRegion(b->stageTex[eye],0,0,0,0,srcTex,0,&box);
    }
    LARGE_INTEGER t1{}; QueryPerformanceCounter(&t1);
    LARGE_INTEGER freq{}; QueryPerformanceFrequency(&freq);
    if (freq.QuadPart) {
        b->msStage += 1000.0 * (double)(t1.QuadPart - t0.QuadPart) / (double)freq.QuadPart;
        ++b->stageFrames;
    }
    const bool sharpenOnly = (b->mode == nis_upscale::kNISSharpen);
    NisTimingBegin(&b->rt, ctx);
    const float sharp = NisBridge_Sharpness(b);
    const bool ok0 = NisDispatchEye(&b->rt, ctx, b->stageSrv[0],
        sbsW/2, sbsH, 0, eyeOutW, eyeOutH, sharp, sharpenOnly);
    const bool ok1 = NisDispatchEye(&b->rt, ctx, b->stageSrv[1],
        sbsW/2, sbsH, 1, eyeOutW, eyeOutH, sharp, sharpenOnly);
    NisTimingEnd(&b->rt, ctx);
    return ok0 && ok1;
}

// Harvest timestamps without stalling; accumulates MEASURED GPU time.
static void NisBridge_PollTiming(NisBridge* b, ID3D11DeviceContext* ctx) {
    if (!ctx) return;
    NisTimingPoll(&b->rt, ctx);
    if (b->rt.timingFrames != b->lastTimingFrames) {
        b->lastTimingFrames = b->rt.timingFrames;
        b->msGpu += b->rt.msGpu;
        if (b->rt.msGpu > b->msGpuPeak) b->msGpuPeak = b->rt.msGpu;
        ++b->gpuFrames;
    }
}

static void NisBridge_ResetStats(NisBridge* b) {
    b->msGpu = b->msGpuPeak = 0.0;
    b->gpuFrames = 0;
    b->msStage = 0.0;
    b->stageFrames = 0;
}

static void NisBridge_Shutdown(NisBridge* b) {
    NisShutdown(&b->rt);
    for (int eye=0; eye<2; ++eye) {
        if (b->stageSrv[eye]) { b->stageSrv[eye]->Release(); b->stageSrv[eye] = nullptr; }
        if (b->stageTex[eye]) { b->stageTex[eye]->Release(); b->stageTex[eye] = nullptr; }
    }
    b->stageW = b->stageH = 0;
}
