#pragma once
// Desktop/TV mirror: aspect-fit one SBS half after XR capture/submit.
// Wire from Hook_Present: ApplyDesktopMirror(dev) immediately before g_origPresent().
#include <windows.h>
#include <d3d9.h>

void Log(_Printf_format_string_ const char* fmt, ...);

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

// Bounded outcome diagnostics: counters + last geometry only, one Log line
// per 600 Apply calls. Mirror copy logic below is unchanged by this probe.
struct Diag {
    static inline volatile LONG calls = 0;
    static inline volatile LONG skipMode = 0;
    static inline volatile LONG skipDup = 0;
    static inline volatile LONG skipRect = 0;
    static inline volatile LONG skipXr = 0;
    static inline volatile LONG failBack = 0;
    static inline volatile LONG failDesc = 0;
    static inline volatile LONG failFit = 0;
    static inline volatile LONG failScratch = 0;
    static inline volatile LONG failStretch1 = 0;
    static inline volatile LONG failFill = 0;
    static inline volatile LONG failStretch2 = 0;
    static inline volatile LONG ok = 0;
    static inline LONG lastMode = -1;
    static inline LONG lastHr = 0;
    static inline UINT lastBw = 0, lastBh = 0, lastWw = 0, lastWh = 0;
    static inline UINT lastSrcLeft = 0;
    static inline RECT lastDst{ 0, 0, 0, 0 };
};

inline void EmitMaybe() {
    const LONG n = InterlockedIncrement(&Diag::calls);
    if (n % 600 != 0) return;
    Log("mirror-diag: n=600 mode=%ld skipMode=%ld skipDup=%ld skipRect=%ld skipXr=%ld "
        "failBack=%ld failDesc=%ld failFit=%ld failScratch=%ld failS1=%ld failFill=%ld failS2=%ld ok=%ld "
        "lastHr=0x%08lX bb=%ux%u win=%ux%u srcLeft=%u dst=(%ld,%ld,%ld,%ld)",
        Diag::lastMode,
        (LONG)InterlockedExchange(&Diag::skipMode, 0), (LONG)InterlockedExchange(&Diag::skipDup, 0),
        (LONG)InterlockedExchange(&Diag::skipRect, 0), (LONG)InterlockedExchange(&Diag::skipXr, 0),
        (LONG)InterlockedExchange(&Diag::failBack, 0), (LONG)InterlockedExchange(&Diag::failDesc, 0),
        (LONG)InterlockedExchange(&Diag::failFit, 0), (LONG)InterlockedExchange(&Diag::failScratch, 0),
        (LONG)InterlockedExchange(&Diag::failStretch1, 0), (LONG)InterlockedExchange(&Diag::failFill, 0),
        (LONG)InterlockedExchange(&Diag::failStretch2, 0), (LONG)InterlockedExchange(&Diag::ok, 0),
        (long)Diag::lastHr, Diag::lastBw, Diag::lastBh, Diag::lastWw, Diag::lastWh,
        Diag::lastSrcLeft, Diag::lastDst.left, Diag::lastDst.top,
        Diag::lastDst.right, Diag::lastDst.bottom);
}

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
    Diag::lastMode = mode;
    const LONG dup = InterlockedCompareExchange(&g_dupDraws, 0, 0);
    if (mode <= 0) { InterlockedIncrement(&Diag::skipMode); EmitMaybe(); return; }
    if (!dup) { InterlockedIncrement(&Diag::skipDup); EmitMaybe(); return; }
    // Subrectangle Present needs a separate mapping contract: leave it native.
    if (!dev || presentSrc || presentDst) {
        InterlockedIncrement(&Diag::skipRect); EmitMaybe(); return;
    }
    if (!g_xrRunning || !g_srcW || !g_srcH) {
        InterlockedIncrement(&Diag::skipXr); EmitMaybe(); return;
    }

    IDirect3DSurface9* back = nullptr;
    HRESULT hr = dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back);
    if (FAILED(hr) || !back) {
        Diag::lastHr = (LONG)hr;
        InterlockedIncrement(&Diag::failBack); EmitMaybe(); return;
    }
    D3DSURFACE_DESC desc{};
    hr = back->GetDesc(&desc);
    if (FAILED(hr) || desc.Width < 2 || !desc.Height) {
        Diag::lastHr = (LONG)hr;
        back->Release();
        InterlockedIncrement(&Diag::failDesc); EmitMaybe(); return;
    }
    Diag::lastBw = desc.Width; Diag::lastBh = desc.Height;

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
    Diag::lastWw = ww; Diag::lastWh = wh;
    RECT dst{};
    if (!FitDestination(desc.Width, desc.Height, desc.Width / 2, desc.Height, ww, wh, dst)) {
        back->Release();
        InterlockedIncrement(&Diag::failFit); EmitMaybe(); return;
    }
    if (!g_scratch || g_scratchW != desc.Width / 2 || g_scratchH != desc.Height ||
        g_scratchFormat != desc.Format) {
        ReleaseScratch();
        hr = dev->CreateRenderTarget(desc.Width / 2, desc.Height, desc.Format, D3DMULTISAMPLE_NONE,
                                     0, FALSE, &g_scratch, nullptr);
        if (FAILED(hr)) {
            Diag::lastHr = (LONG)hr;
            back->Release();
            InterlockedIncrement(&Diag::failScratch); EmitMaybe(); return;
        }
        g_scratchW = desc.Width / 2;
        g_scratchH = desc.Height;
        g_scratchFormat = desc.Format;
    }

    const bool swap = InterlockedCompareExchange(&g_swapEyes, 0, 0) != 0;
    const UINT srcLeft = SourceLeft(desc.Width, mode, swap, g_scCropped);
    Diag::lastSrcLeft = srcLeft;
    Diag::lastDst = dst;
    const RECT src{ (LONG)srcLeft, 0, (LONG)(srcLeft + desc.Width / 2), (LONG)desc.Height };
    hr = dev->StretchRect(back, &src, g_scratch, nullptr, D3DTEXF_LINEAR);
    if (FAILED(hr)) {
        Diag::lastHr = (LONG)hr;
        back->Release();
        InterlockedIncrement(&Diag::failStretch1); EmitMaybe(); return;
    }
    hr = dev->ColorFill(back, nullptr, D3DCOLOR_XRGB(0, 0, 0));
    if (FAILED(hr)) {
        Diag::lastHr = (LONG)hr;
        back->Release();
        InterlockedIncrement(&Diag::failFill); EmitMaybe(); return;
    }
    hr = dev->StretchRect(g_scratch, nullptr, back, &dst, D3DTEXF_LINEAR);
    if (FAILED(hr)) {
        Diag::lastHr = (LONG)hr;
        back->Release();
        InterlockedIncrement(&Diag::failStretch2); EmitMaybe(); return;
    }
    InterlockedIncrement(&Diag::ok);
    back->Release();
    EmitMaybe();
}

} // namespace desktop_mirror
