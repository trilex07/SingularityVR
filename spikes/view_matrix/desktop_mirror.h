#pragma once
// Desktop/TV mirror: aspect-fit one SBS half after XR capture/submit.
// Wire from Hook_Present: ApplyDesktopMirror(dev) immediately before g_origPresent().
#include <windows.h>
#include <d3d9.h>

extern volatile LONG g_dupDraws;
extern volatile LONG g_swapEyes;
extern volatile LONG g_desktopMirrorEye;
extern bool g_scCropped;
extern bool g_xrRunning;
extern unsigned g_srcW;
extern unsigned g_srcH;

namespace desktop_mirror {

inline IDirect3DSurface9* g_scratch = nullptr;
inline UINT g_scratchW = 0;
inline UINT g_scratchH = 0;
inline D3DFORMAT g_scratchFormat = D3DFMT_UNKNOWN;

inline void ReleaseScratch() {
    if (g_scratch) {
        g_scratch->Release();
        g_scratch = nullptr;
    }
    g_scratchW = g_scratchH = 0;
    g_scratchFormat = D3DFMT_UNKNOWN;
}

// Account for Present scaling the backbuffer to a differently shaped window.
// The resulting rectangle is in BACKBUFFER pixels, but preserves source aspect
// in WINDOW pixels. No eye pixels are cropped to fill a widescreen television.
inline bool FitDestination(UINT bw, UINT bh, UINT sw, UINT sh,
                           UINT ww, UINT wh, RECT& out) {
    if (!bw || !bh || !sw || !sh || !ww || !wh ||
        bw > 0x7fffffffU || bh > 0x7fffffffU) return false;
    const double aspect = double(sw) / sh;
    double w = ww, h = w / aspect;
    if (h > wh) { h = wh; w = h * aspect; }
    LONG dw = LONG(w * bw / ww + .5), dh = LONG(h * bh / wh + .5);
    if (dw < 1 || dh < 1 || dw > LONG(bw) || dh > LONG(bh)) return false;
    out = { (LONG(bw) - dw) / 2, (LONG(bh) - dh) / 2,
            (LONG(bw) - dw) / 2 + dw, (LONG(bh) - dh) / 2 + dh };
    return true;
}

// mode 1 = left eye, mode 2 = right eye. Swap follows the same half the XR
// bridge uses. A cropped menu/cutscene frame already repeated one half, so
// the mirror takes that half instead of the empty one.
inline UINT SourceLeft(UINT width, LONG mode, bool swap, bool cropped) {
    const UINT half = width / 2;
    if (cropped) return swap ? half : 0;
    const bool right = mode == 2;
    const UINT halfIndex = right ? (swap ? 0u : 1u) : (swap ? 1u : 0u);
    return halfIndex * half;
}

inline void Apply(IDirect3DDevice9* dev, const RECT* presentSrc,
                  const RECT* presentDst, HWND overrideWindow) {
    const LONG mode = InterlockedCompareExchange(&g_desktopMirrorEye, 0, 0);
    if (mode <= 0 || !InterlockedCompareExchange(&g_dupDraws, 0, 0)) return;
    // Subrectangle Present needs a separate mapping contract: leave it native.
    if (!dev || presentSrc || presentDst || !g_xrRunning || !g_srcW || !g_srcH) return;

    IDirect3DSurface9* back = nullptr;
    if (FAILED(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back)) || !back) return;
    D3DSURFACE_DESC desc{};
    if (FAILED(back->GetDesc(&desc)) || desc.Width < 2 || !desc.Height) {
        back->Release();
        return;
    }

    D3DDEVICE_CREATION_PARAMETERS creation{};
    D3DPRESENT_PARAMETERS params{};
    IDirect3DSwapChain9* chain = nullptr;
    HWND window = overrideWindow;
    if (!window && SUCCEEDED(dev->GetSwapChain(0, &chain)) && chain) {
        if (SUCCEEDED(chain->GetPresentParameters(&params))) window = params.hDeviceWindow;
    }
    if (chain) chain->Release();
    if (!window && SUCCEEDED(dev->GetCreationParameters(&creation))) window = creation.hFocusWindow;
    RECT client{};
    UINT ww = desc.Width, wh = desc.Height;
    if (window && GetClientRect(window, &client) && client.right > 0 && client.bottom > 0) {
        ww = UINT(client.right); wh = UINT(client.bottom);
    }
    RECT dst{};
    if (!FitDestination(desc.Width, desc.Height, desc.Width / 2, desc.Height, ww, wh, dst)) {
        back->Release(); return;
    }
    if (!g_scratch || g_scratchW != desc.Width / 2 || g_scratchH != desc.Height ||
        g_scratchFormat != desc.Format) {
        ReleaseScratch();
        if (FAILED(dev->CreateRenderTarget(desc.Width / 2, desc.Height, desc.Format, D3DMULTISAMPLE_NONE,
                                           0, FALSE, &g_scratch, nullptr))) {
            back->Release();
            return;
        }
        g_scratchW = desc.Width / 2;
        g_scratchH = desc.Height;
        g_scratchFormat = desc.Format;
    }

    const bool swap = InterlockedCompareExchange(&g_swapEyes, 0, 0) != 0;
    const UINT srcLeft = SourceLeft(desc.Width, mode, swap, g_scCropped);
    const RECT src{ (LONG)srcLeft, 0, (LONG)(srcLeft + desc.Width / 2), (LONG)desc.Height };
    if (SUCCEEDED(dev->StretchRect(back, &src, g_scratch, nullptr, D3DTEXF_LINEAR))) {
        if (SUCCEEDED(dev->ColorFill(back, nullptr, D3DCOLOR_XRGB(0, 0, 0))))
            dev->StretchRect(g_scratch, nullptr, back, &dst, D3DTEXF_LINEAR);
    }
    back->Release();
}

} // namespace desktop_mirror
