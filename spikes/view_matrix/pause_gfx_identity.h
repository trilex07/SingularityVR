#pragma once
// MDG-backed Scaleform flash identity (session pointers + vertex declaration), not stride/blend alone.
// Physical class 2/3: stride 12 ps+decl wide batch, stride 8 narrow batch; blend SRCALPHA, Z off.
#include "pause_gfx_signature.h"
#include <d3d9.h>

namespace pausegfx {

struct FlashShaderSlot {
    volatile LONG ps = 0;
    volatile LONG vs = 0;
    volatile LONG decl = 0;
};

static FlashShaderSlot g_flashWide{};
static FlashShaderSlot g_flashNarrow{};

inline void ResetLearnedFlashIdentity() {
    InterlockedExchange(&g_flashWide.ps, 0);
    InterlockedExchange(&g_flashWide.vs, 0);
    InterlockedExchange(&g_flashWide.decl, 0);
    InterlockedExchange(&g_flashNarrow.ps, 0);
    InterlockedExchange(&g_flashNarrow.vs, 0);
    InterlockedExchange(&g_flashNarrow.decl, 0);
}

inline bool FlashStructuralDraw(IDirect3DDevice9* dev, UINT stride, UINT primCount) {
    const int slot = FlashStrideSlot(stride);
    if (slot < 0 || primCount == 0 || primCount > kFlashPrimMax) return false;
    return FlashBlendAndDepth(dev);
}

inline bool ReadFlashShaders(IDirect3DDevice9* dev, LONG& ps, LONG& vs, LONG& decl) {
    ps = vs = decl = 0;
    if (!dev) return false;
    IDirect3DPixelShader9* p = nullptr;
    IDirect3DVertexShader9* v = nullptr;
    IDirect3DVertexDeclaration9* d = nullptr;
    if (FAILED(dev->GetPixelShader(&p)) || !p) return false;
    if (FAILED(dev->GetVertexShader(&v)) || !v) {
        p->Release();
        return false;
    }
    if (FAILED(dev->GetVertexDeclaration(&d)) || !d) {
        p->Release();
        v->Release();
        return false;
    }
    ps = (LONG)(uintptr_t)p;
    vs = (LONG)(uintptr_t)v;
    decl = (LONG)(uintptr_t)d;
    p->Release();
    v->Release();
    d->Release();
    return true;
}

inline bool FlashIdentityMatchSlot(int slot, LONG ps, LONG vs, LONG decl, bool allowLearn) {
    if (slot < 0) return false;
    FlashShaderSlot* box = slot == 0 ? &g_flashWide : &g_flashNarrow;
    const LONG havePs = InterlockedCompareExchange(&box->ps, 0, 0);
    if (!havePs) {
        if (!allowLearn) return false;
        InterlockedCompareExchange(&box->ps, ps, 0);
        InterlockedCompareExchange(&box->vs, vs, 0);
        InterlockedCompareExchange(&box->decl, decl, 0);
    }
    const LONG gotPs = InterlockedCompareExchange(&box->ps, 0, 0);
    const LONG gotVs = InterlockedCompareExchange(&box->vs, 0, 0);
    const LONG gotDecl = InterlockedCompareExchange(&box->decl, 0, 0);
    return gotPs == ps && gotVs == vs && gotDecl == decl;
}

// allowLearn: true only when PauseHeadRedirectContextActive — never learn from main menu / gameplay.
inline bool FlashIdentityMatchesOrLearn(IDirect3DDevice9* dev, UINT stride, bool allowLearn) {
    LONG ps = 0, vs = 0, decl = 0;
    if (!ReadFlashShaders(dev, ps, vs, decl)) return false;
    return FlashIdentityMatchSlot(FlashStrideSlot(stride), ps, vs, decl, allowLearn);
}

inline void ResetLearnedFlashPs() { ResetLearnedFlashIdentity(); }

}  // namespace pausegfx
