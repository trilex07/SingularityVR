#pragma once
// Structural pre-filter for Scaleform GFx flash DrawIndexedPrimUP batches only (MDG 74/137/140/263).
// Does NOT identify the full pause list: large kind=1 stride-12 indexed paths are excluded on purpose.
#include <d3d9.h>

namespace pausegfx {

constexpr UINT kFlashStrideWide = 12;
constexpr UINT kFlashStrideNarrow = 8;
constexpr UINT kFlashPrimMax = 512;

inline int FlashStrideSlot(UINT stride) {
    if (stride == kFlashStrideWide) return 0;
    if (stride == kFlashStrideNarrow) return 1;
    return -1;
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

}  // namespace pausegfx
