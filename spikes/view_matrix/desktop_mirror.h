#pragma once
// Desktop/TV mirror: stretch one SBS half to the full game window after XR submit.
// Wire from Hook_Present: ApplyDesktopMirror(dev) immediately before g_origPresent().
#include <windows.h>
#include <d3d9.h>

extern volatile LONG g_dupDraws;
extern volatile LONG g_swapEyes;
extern volatile LONG g_desktopMirrorEye;
extern bool g_scCropped;
extern bool g_xrRunning;
extern uint32_t g_srcW;
extern uint32_t g_srcH;

namespace desktop_mirror {

inline IDirect3DSurface9* g_scratch = nullptr;
inline UINT g_scratchW = 0;
inline UINT g_scratchH = 0;

inline void ReleaseScratch() {
    if (g_scratch) {
        g_scratch->Release();
        g_scratch = nullptr;
    }
    g_scratchW = g_scratchH = 0;
}

inline void Apply(IDirect3DDevice9* dev) {
    const LONG mode = InterlockedCompareExchange(&g_desktopMirrorEye, 0, 0);
    if (mode <= 0 || !InterlockedCompareExchange(&g_dupDraws, 0, 0)) return;
    if (!g_xrRunning || !g_srcW || !g_srcH) return;

    IDirect3DSurface9* back = nullptr;
    if (FAILED(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back)) || !back) return;
    D3DSURFACE_DESC desc{};
    if (FAILED(back->GetDesc(&desc)) || desc.Width < 2) {
        back->Release();
        return;
    }

    if (!g_scratch || g_scratchW != desc.Width || g_scratchH != desc.Height) {
        ReleaseScratch();
        if (FAILED(dev->CreateRenderTarget(desc.Width, desc.Height, desc.Format, D3DMULTISAMPLE_NONE,
                                           0, TRUE, &g_scratch, nullptr))) {
            back->Release();
            return;
        }
        g_scratchW = desc.Width;
        g_scratchH = desc.Height;
    }

    const UINT halfW = desc.Width / 2;
    const bool swap = InterlockedCompareExchange(&g_swapEyes, 0, 0) != 0;
    UINT srcLeft = 0;
    if (g_scCropped) {
        srcLeft = swap ? halfW : 0;
    } else {
        const bool right = (mode == 2);
        const UINT halfIndex = right ? (swap ? 0u : 1u) : (swap ? 1u : 0u);
        srcLeft = halfIndex * halfW;
    }
    const RECT src{ (LONG)srcLeft, 0, (LONG)(srcLeft + halfW), (LONG)desc.Height };
    if (SUCCEEDED(dev->StretchRect(back, &src, g_scratch, nullptr, D3DTEXF_LINEAR))) {
        dev->StretchRect(g_scratch, nullptr, back, nullptr, D3DTEXF_LINEAR);
    }
    back->Release();
}

} // namespace desktop_mirror
