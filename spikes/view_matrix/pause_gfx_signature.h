#pragma once
// Evidence-backed Scaleform GFx flash draw fingerprint from physical MDG (class 2 @ 1280x720,
// class 3 @ scene HDR 5264x2684): DrawIndexedPrimUP, stride 8 or 12, SRCALPHA blend, Z off,
// small batch sizes; ps/decl stable within a process (learned on first class-3 hit).
#include <d3d9.h>
#include <cmath>

namespace pausegfx {

constexpr UINT kFlashStrideWide = 12;
constexpr UINT kFlashStrideNarrow = 8;
constexpr UINT kFlashPrimMax = 512;

inline int FlashStrideSlot(UINT stride) {
    if (stride == kFlashStrideWide) return 0;
    if (stride == kFlashStrideNarrow) return 1;
    return -1;
}

static volatile LONG g_learnedPsWide = 0;
static volatile LONG g_learnedPsNarrow = 0;

inline void ResetLearnedFlashPs() {
    InterlockedExchange(&g_learnedPsWide, 0);
    InterlockedExchange(&g_learnedPsNarrow, 0);
}

inline bool LearnedFlashPsMatches(IDirect3DDevice9* dev, UINT stride) {
    const int slot = FlashStrideSlot(stride);
    if (slot < 0 || !dev) return false;
    IDirect3DPixelShader9* ps = nullptr;
    if (FAILED(dev->GetPixelShader(&ps)) || !ps) return false;
    const LONG p = (LONG)(uintptr_t)ps;
    ps->Release();
    volatile LONG* box = slot == 0 ? &g_learnedPsWide : &g_learnedPsNarrow;
    const LONG prior = InterlockedCompareExchange(box, 0, 0);
    if (!prior) {
        InterlockedCompareExchange(box, p, 0);
        return true;
    }
    return prior == p;
}

inline bool FlashBlendAndDepth(IDirect3DDevice9* dev) {
    if (!dev) return false;
    DWORD blend = 0, src = 0, dst = 0, z = 0, zw = 0;
    if (FAILED(dev->GetRenderState(D3DRS_ALPHABLENDENABLE, &blend))) return false;
    if (FAILED(dev->GetRenderState(D3DRS_SRCBLEND, &src))) return false;
    if (FAILED(dev->GetRenderState(D3DRS_DESTBLEND, &dst))) return false;
    if (FAILED(dev->GetRenderState(D3DRS_ZENABLE, &z))) return false;
    if (FAILED(dev->GetRenderState(D3DRS_ZWRITEENABLE, &zw))) return false;
    return blend != 0 && src == D3DBLEND_SRCALPHA && dst == D3DBLEND_INVSRCALPHA && z == 0 &&
           zw == 0;
}

inline bool FlashDrawIndexedPrimUP(IDirect3DDevice9* dev, UINT stride, UINT primCount) {
    const int slot = FlashStrideSlot(stride);
    if (slot < 0 || primCount == 0 || primCount > kFlashPrimMax) return false;
    if (!FlashBlendAndDepth(dev)) return false;
    return LearnedFlashPsMatches(dev, stride);
}

// Scale GFx row-basis c5-c8 when moving from scene-sized viewport to 1280x720 pool.
inline void ScaleGfxRowConstantsForViewport(float c[16], float fromW, float fromH, float toW,
                                          float toH) {
    if (fromW <= 1.f || fromH <= 1.f || toW <= 1.f || toH <= 1.f) return;
    const float sx = toW / fromW;
    const float sy = toH / fromH;
    float* q = c;
    for (int i = 0; i < 16; i += 4) {
        q[i] *= sx;
        q[i + 1] *= sy;
    }
}

inline void ScaleScissorRect(RECT& r, float sx, float sy) {
    r.left = (LONG)lroundf((float)r.left * sx);
    r.right = (LONG)lroundf((float)r.right * sx);
    r.top = (LONG)lroundf((float)r.top * sy);
    r.bottom = (LONG)lroundf((float)r.bottom * sy);
}

}  // namespace pausegfx
