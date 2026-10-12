#pragma once
#include "pause_canvas_backbuffer_trace.h"
#include "pause_menu_mode.h"
#include "menu_capture_owner.h"
#include <d3d9.h>

extern void Log(const char* fmt, ...);
extern volatile LONG g_gamePaused;
extern volatile LONG g_pauseCaptureOk;
extern volatile LONG g_pauseMenuMode;
extern volatile LONG g_panelOpen;
extern menucapture::Owner g_vrCaptureOwner;
extern HRESULT(STDMETHODCALLTYPE* g_origSetRenderTarget)(IDirect3DDevice9*, DWORD, IDirect3DSurface9*);

void PauseCanvasRegisterCapture(std::uint64_t token, unsigned bandA, unsigned bandB, unsigned rtW,
                                unsigned rtH);

namespace pausecanvas {
namespace capture {

static IDirect3DTexture9* g_poolTex = nullptr;
static IDirect3DSurface9* g_poolSurf = nullptr;
static IDirect3DStateBlock9* g_state = nullptr;
static unsigned g_poolW = 0, g_poolH = 0;

struct FrameLedger {
    unsigned key = ~0u;
    std::uint64_t token = 0;
    unsigned bandA = 0, bandB = 0, mid = 0, notOrtho = 0;
    std::uintptr_t backSurf = 0;
    bool incompatible = false;
    bool cleared = false;
};
static FrameLedger g_ledger{};

inline void ReleasePool() {
    if (g_state) {
        g_state->Release();
        g_state = nullptr;
    }
    if (g_poolSurf) {
        g_poolSurf->Release();
        g_poolSurf = nullptr;
    }
    if (g_poolTex) {
        g_poolTex->Release();
        g_poolTex = nullptr;
    }
    g_poolW = g_poolH = 0;
    g_ledger = {};
}

inline bool EnsurePool(IDirect3DDevice9* dev, unsigned w, unsigned h) {
    if (!dev || w < 640 || h < 360) return false;
    if (g_poolTex && g_poolW == w && g_poolH == h) return true;
    ReleasePool();
    IDirect3DTexture9* tex = nullptr;
    const HRESULT hr =
        dev->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex, nullptr);
    if (FAILED(hr) || !tex) return false;
    IDirect3DSurface9* surf = nullptr;
    if (FAILED(tex->GetSurfaceLevel(0, &surf))) {
        tex->Release();
        return false;
    }
    g_poolTex = tex;
    g_poolSurf = surf;
    g_poolW = w;
    g_poolH = h;
    static LONG said = 0;
    if (InterlockedCompareExchange(&said, 1, 0) == 0)
        Log("pause-canvas-cap: pool %ux%u A8R8G8B8 ready", w, h);
    return true;
}

inline bool ActiveMode() {
    return pausemode::CapturesCanvasBackbuffer((int)InterlockedCompareExchange(&g_pauseMenuMode, 0, 0));
}

inline bool RouteEligible(int menuClass, bool panelOpen) {
    return menuClass == 3 && InterlockedCompareExchange(&g_gamePaused, 0, 0) &&
           !panelOpen && ActiveMode();
}

inline void FinalizeFrame(unsigned frameKey) {
    if (g_ledger.key != frameKey) return;
    if (g_ledger.incompatible) {
        static LONG said = 0;
        if (InterlockedCompareExchange(&said, 1, 0) == 0)
            Log("pause-canvas-cap: reject frame=%u reason=surf_mismatch back=%p",
                frameKey, (void*)g_ledger.backSurf);
        InterlockedExchange(&g_pauseCaptureOk, 0);
        g_ledger.key = ~0u;
        return;
    }
    const bool settled = g_ledger.bandA > 0 && g_ledger.bandB > 0;
    if (settled) {
        PauseCanvasRegisterCapture(g_ledger.token, g_ledger.bandA, g_ledger.bandB, g_poolW, g_poolH);
        Log("pause-canvas-cap: frame=%u token=%llX bandA=%u bandB=%u surf=pool %ux%u settled=1",
            frameKey, (unsigned long long)g_ledger.token, g_ledger.bandA, g_ledger.bandB, g_poolW,
            g_poolH);
    } else {
        Log("pause-canvas-cap: frame=%u token=%llX bandA=%u bandB=%u settled=0 (legacy only)",
            frameKey, (unsigned long long)g_ledger.token, g_ledger.bandA, g_ledger.bandB);
    }
    g_ledger.key = ~0u;
}

inline void BeginFrame(unsigned frameKey, std::uint64_t token) {
    if (g_ledger.key != ~0u && g_ledger.key != frameKey) FinalizeFrame(g_ledger.key);
    if (g_ledger.key != frameKey) {
        g_ledger = {};
        g_ledger.key = frameKey;
        g_ledger.token = token;
    }
}

inline void FinishPresentFrame(unsigned frameKey) { FinalizeFrame(frameKey); }

inline IDirect3DTexture9* PoolTexture() { return g_poolTex; }

inline bool SuppressNativeDraws(std::uint64_t token) {
    const bool panelOpen = InterlockedCompareExchange(&g_panelOpen, 0, 0) != 0;
    if (!ActiveMode() || panelOpen) return false;
    if (!InterlockedCompareExchange(&g_pauseCaptureOk, 0, 0)) return false;
    return g_vrCaptureOwner.CanProvide(token);
}

template <typename DrawFn>
inline HRESULT MirrorBatch(IDirect3DDevice9* dev, const float c5[16], DrawFn&& draw,
                           std::uint64_t token, unsigned frameKey, unsigned bbW, unsigned bbH) {
    const bool panelOpen = InterlockedCompareExchange(&g_panelOpen, 0, 0) != 0;
    if (!RouteEligible(3, panelOpen) || !dev || !g_origSetRenderTarget) return draw();
    const Band band = ScaleBand(c5[0], pausemenuspace::IsPauseCanvasOrtho(c5));
    if (band == Band::NotOrtho) {
        ++g_ledger.notOrtho;
        return draw();
    }
    if (band == Band::Mid) {
        ++g_ledger.mid;
        return draw();
    }
    BeginFrame(frameKey, token);
    if (!EnsurePool(dev, bbW, bbH)) return draw();

    IDirect3DSurface9* rt = nullptr;
    if (FAILED(dev->GetRenderTarget(0, &rt)) || !rt) return draw();
    const std::uintptr_t surf = (std::uintptr_t)rt;
    rt->Release();
    if (!g_ledger.backSurf) g_ledger.backSurf = surf;
    else if (g_ledger.backSurf != surf) {
        g_ledger.incompatible = true;
        return draw();
    }

    if (!g_ledger.cleared) {
        if (!g_state && FAILED(dev->CreateStateBlock(D3DSBT_ALL, &g_state))) return draw();
        if (FAILED(g_state->Capture())) return draw();
        g_origSetRenderTarget(dev, 0, g_poolSurf);
        D3DVIEWPORT9 vp{0, 0, bbW, bbH, 0, 1};
        dev->SetViewport(&vp);
        RECT sc{0, 0, (LONG)bbW, (LONG)bbH};
        dev->SetScissorRect(&sc);
        dev->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
        dev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.f, 0);
        g_ledger.cleared = true;
        g_state->Apply();
    }

    if (!g_state && FAILED(dev->CreateStateBlock(D3DSBT_ALL, &g_state))) return draw();
    if (FAILED(g_state->Capture())) return draw();

    g_origSetRenderTarget(dev, 0, g_poolSurf);
    D3DVIEWPORT9 vp{0, 0, bbW, bbH, 0, 1};
    dev->SetViewport(&vp);
    RECT sc{0, 0, (LONG)bbW, (LONG)bbH};
    dev->SetScissorRect(&sc);
    dev->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
    dev->SetVertexShaderConstantF(5, c5, 4);

    const HRESULT hr = draw();

    if (band == Band::A) ++g_ledger.bandA;
    if (band == Band::B) ++g_ledger.bandB;

    g_state->Apply();
    return hr;
}

}  // namespace capture
}  // namespace pausecanvas
