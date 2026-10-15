#pragma once
// Desktop/TV mirror: aspect-fit one SBS half after XR capture/submit.
// Wire from Hook_Present: ApplyDesktopMirror(dev) immediately before g_origPresent().
#include <windows.h>
#include <d3d9.h>

void Log(_Printf_format_string_ const char* fmt, ...);

extern volatile LONG g_dupDraws;
extern volatile LONG g_swapEyes;
extern volatile LONG g_desktopMirrorEye;
extern volatile LONG g_desktopMirrorFill;
extern volatile LONG g_inCinematic;
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
    // One-shot subrectangle probe: first 4 distinct (src,dst) Present rect
    // shapes per session. Read-only; never touches pixels.
    struct RectSample { RECT src{}; RECT dst{}; bool hasSrc = false; bool hasDst = false;
                        UINT bw = 0, bh = 0, ww = 0, wh = 0; };
    static inline RectSample rectSamples[4]{};
    static inline LONG rectCount = 0;
    // One-shot window/monitor placement probe (max 2 logs/session, read-only).
    static inline volatile LONG placeCount = 0;
};

inline void CapturePlacement(HWND window) {
    const LONG slot = InterlockedIncrement(&Diag::placeCount);
    if (slot != 1 && slot != 2) return;
    RECT wr{ 0, 0, 0, 0 };
    if (window) GetWindowRect(window, &wr);
    RECT mr{ 0, 0, 0, 0 };
    if (window) {
        HMONITOR mon = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
        if (mon) {
            MONITORINFO mi{};
            mi.cbSize = sizeof(mi);
            if (GetMonitorInfoA(mon, &mi)) mr = mi.rcMonitor;
        }
    }
    Log("mirror-place: win=(%ld,%ld,%ld,%ld) mon=(%ld,%ld,%ld,%ld)",
        wr.left, wr.top, wr.right, wr.bottom,
        mr.left, mr.top, mr.right, mr.bottom);
}

inline void CaptureRects(IDirect3DDevice9* dev, const RECT* presentSrc,
                         const RECT* presentDst, HWND overrideWindow) {
    Diag::RectSample cur{};
    cur.hasSrc = presentSrc != nullptr;
    cur.hasDst = presentDst != nullptr;
    if (presentSrc) cur.src = *presentSrc;
    if (presentDst) cur.dst = *presentDst;
    if (dev) {
        IDirect3DSurface9* back = nullptr;
        if (SUCCEEDED(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back)) && back) {
            D3DSURFACE_DESC desc{};
            if (SUCCEEDED(back->GetDesc(&desc))) { cur.bw = desc.Width; cur.bh = desc.Height; }
            back->Release();
        }
    }
    D3DPRESENT_PARAMETERS params{};
    IDirect3DSwapChain9* chain = nullptr;
    HWND window = overrideWindow;
    if (!window && dev && SUCCEEDED(dev->GetSwapChain(0, &chain)) && chain) {
        if (SUCCEEDED(chain->GetPresentParameters(&params))) window = params.hDeviceWindow;
    }
    if (chain) chain->Release();
    if (!window && dev) {
        D3DDEVICE_CREATION_PARAMETERS creation{};
        if (SUCCEEDED(dev->GetCreationParameters(&creation))) window = creation.hFocusWindow;
    }
    RECT client{};
    if (window && GetClientRect(window, &client) && client.right > 0 && client.bottom > 0) {
        cur.ww = UINT(client.right); cur.wh = UINT(client.bottom);
    }
    for (LONG i = 0; i < Diag::rectCount && i < 4; ++i) {
        const Diag::RectSample& s = Diag::rectSamples[i];
        if (s.hasSrc == cur.hasSrc && s.hasDst == cur.hasDst &&
            (!cur.hasSrc || (s.src.left == cur.src.left && s.src.top == cur.src.top &&
                             s.src.right == cur.src.right && s.src.bottom == cur.src.bottom)) &&
            (!cur.hasDst || (s.dst.left == cur.dst.left && s.dst.top == cur.dst.top &&
                             s.dst.right == cur.dst.right && s.dst.bottom == cur.dst.bottom)) &&
            s.bw == cur.bw && s.bh == cur.bh && s.ww == cur.ww && s.wh == cur.wh)
            return;
    }
    const LONG slot = InterlockedIncrement(&Diag::rectCount) - 1;
    if (slot < 0 || slot >= 4) return;
    Diag::rectSamples[slot] = cur;
    Log("mirror-rect: src=%s(%ld,%ld,%ld,%ld) dst=%s(%ld,%ld,%ld,%ld) bb=%ux%u win=%ux%u",
        cur.hasSrc ? "" : "none", cur.src.left, cur.src.top, cur.src.right, cur.src.bottom,
        cur.hasDst ? "" : "none", cur.dst.left, cur.dst.top, cur.dst.right, cur.dst.bottom,
        cur.bw, cur.bh, cur.ww, cur.wh);
}

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
inline UINT SourceLeft(UINT width, LONG mode, bool swap, bool cropped, bool monoOrCutscene = false) {
    const UINT half = width / 2;
    if (cropped || monoOrCutscene) return swap ? half : 0;
    const bool right = mode == 2;
    const UINT halfIndex = right ? (swap ? 0u : 1u) : (swap ? 1u : 0u);
    return halfIndex * half;
}

// Subrectangle contract (runtime-proven 2026-10-14: engine passes a full-
// backbuffer pSourceRect with NULL pDestRect on every VR Present).
// A source rect covering the whole backbuffer with no dest rect is
// equivalent to NULL: Apply() below only uses backbuffer dimensions.
// Any true subrectangle stays native and is sampled by the probe.
inline bool IsFullBackbufferSrc(const RECT* presentSrc, const RECT* presentDst,
                                UINT bw, UINT bh) {
    if (presentDst) return false;
    if (!presentSrc) return true;
    return presentSrc->left <= 0 && presentSrc->top <= 0 &&
           presentSrc->right >= (LONG)bw && presentSrc->bottom >= (LONG)bh;
}

// Fit one eye into a visible client sub-box (all in screen pixels except the
// backbuffer scale), returning a backbuffer-pixel dst for StretchRect.
// Falls back to full-backbuffer centering when nothing is visible.
inline bool VisibleEyeDst(LONG visX, LONG visY, LONG visW, LONG visH,
                          UINT bw, UINT bh, UINT ww, UINT wh,
                          UINT sw, UINT sh, RECT& out) {
    if (visW <= 0 || visH <= 0 || !bw || !bh || !sw || !sh || !ww || !wh)
        return FitDestination(bw, bh, sw, sh, ww, wh, out);
    const double sx = double(bw) / ww, sy = double(bh) / wh;
    // Eye aspect as displayed on screen (backbuffer-to-window scale applied).
    const double aspect = (double(sw) * ww / bw) / (double(sh) * wh / bh);
    double w = double(visW), h = w / aspect;
    if (h > double(visH)) { h = double(visH); w = h * aspect; }
    if (w < 1 || h < 1) return false;
    const double ox = double(visX) + (double(visW) - w) / 2;
    const double oy = double(visY) + (double(visH) - h) / 2;
    const LONG l = LONG(ox * sx + .5), t = LONG(oy * sy + .5);
    out = { l, t, l + LONG(w * sx + .5), t + LONG(h * sy + .5) };
    return out.right <= (LONG)bw && out.bottom <= (LONG)bh && out.left >= 0 && out.top >= 0;
}

// Visible client sub-box in client pixels (intersection of the window client
// area with the nearest monitor). False when nothing is visible.
inline bool VisibleClientBox(HWND window, UINT ww, UINT wh,
                             LONG& vx, LONG& vy, LONG& vw, LONG& vh) {
    vx = 0; vy = 0; vw = (LONG)ww; vh = (LONG)wh;
    if (!window || !ww || !wh) return false;
    POINT org{ 0, 0 };
    if (!ClientToScreen(window, &org)) return false;
    HMONITOR mon = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
    RECT mr{ org.x, org.y, org.x + (LONG)ww, org.y + (LONG)wh };
    if (mon) {
        MONITORINFO mi{};
        mi.cbSize = sizeof(mi);
        if (GetMonitorInfoA(mon, &mi)) mr = mi.rcMonitor;
    }
    const LONG l = org.x > mr.left ? org.x : mr.left;
    const LONG t = org.y > mr.top ? org.y : mr.top;
    const LONG r = (org.x + (LONG)ww) < mr.right ? (org.x + (LONG)ww) : mr.right;
    const LONG b = (org.y + (LONG)wh) < mr.bottom ? (org.y + (LONG)wh) : mr.bottom;
    if (r <= l || b <= t) return false;
    vx = l - org.x; vy = t - org.y; vw = r - l; vh = b - t;
    return true;
}

// Fit/fill presentation geometry:
// fillMode 0 = aspect-fit with black margins (default legacy)
// fillMode 1 = full backbuffer stretch (fills 100% of backbuffer, no black borders on F12/TV)
// fillMode 2 = aspect-crop 16:9 (center-crops source eye to match backbuffer aspect, fills 100% with zero distortion)
inline bool ComputeMirrorDestination(UINT bw, UINT bh, UINT sw, UINT sh,
                                     UINT ww, UINT wh, LONG fillMode,
                                     bool haveVis, LONG vx, LONG vy, LONG vw, LONG vh,
                                     RECT& outDst, RECT& outScratchSrc, bool& outUseScratchSrc, bool& outNeedClear) {
    if (!bw || !bh || !sw || !sh) return false;
    outUseScratchSrc = false;
    outNeedClear = true;
    if (fillMode == 1) {
        outDst = { 0, 0, (LONG)bw, (LONG)bh };
        outNeedClear = false;
        return true;
    }
    if (fillMode == 2) {
        outDst = { 0, 0, (LONG)bw, (LONG)bh };
        outNeedClear = false;
        const double targetAspect = double(bw) / double(bh);
        const double eyeAspect = double(sw) / double(sh);
        if (targetAspect > eyeAspect) {
            const UINT cropH = (UINT)(double(sw) / targetAspect + 0.5);
            const UINT cropTop = (sh > cropH) ? (sh - cropH) / 2 : 0;
            outScratchSrc = { 0, (LONG)cropTop, (LONG)sw, (LONG)(cropTop + cropH) };
            outUseScratchSrc = true;
        } else {
            const UINT cropW = (UINT)(double(sh) * targetAspect + 0.5);
            const UINT cropLeft = (sw > cropW) ? (sw - cropW) / 2 : 0;
            outScratchSrc = { (LONG)cropLeft, 0, (LONG)(cropLeft + cropW), (LONG)sh };
            outUseScratchSrc = true;
        }
        return true;
    }
    return haveVis
        ? VisibleEyeDst(vx, vy, vw, vh, bw, bh, ww, wh, sw, sh, outDst)
        : FitDestination(bw, bh, sw, sh, ww, wh, outDst);
}

inline void Apply(IDirect3DDevice9* dev, const RECT* presentSrc,
                  const RECT* presentDst, HWND overrideWindow) {
    const LONG mode = InterlockedCompareExchange(&g_desktopMirrorEye, 0, 0);
    Diag::lastMode = mode;
    const LONG dup = InterlockedCompareExchange(&g_dupDraws, 0, 0);
    if (mode <= 0) { InterlockedIncrement(&Diag::skipMode); EmitMaybe(); return; }
    if (!dup && !g_xrRunning) { InterlockedIncrement(&Diag::skipDup); EmitMaybe(); return; }
    if (!dev) { InterlockedIncrement(&Diag::skipRect); EmitMaybe(); return; }

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
    // Subrectangle contract (runtime-proven 2026-10-14: engine passes a full-
    // backbuffer pSourceRect with NULL pDestRect on every VR Present).
    // A source rect covering the whole backbuffer with no dest rect is
    // equivalent to NULL: the mirror below only uses backbuffer dimensions.
    // Any true subrectangle stays native and is sampled by the probe.
    if (!IsFullBackbufferSrc(presentSrc, presentDst, desc.Width, desc.Height)) {
        if (dev) CaptureRects(dev, presentSrc, presentDst, overrideWindow);
        back->Release();
        InterlockedIncrement(&Diag::skipRect); EmitMaybe(); return;
    }
    if (!g_xrRunning || !g_srcW || !g_srcH) {
        back->Release();
        InterlockedIncrement(&Diag::skipXr); EmitMaybe(); return;
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
    Diag::lastWw = ww; Diag::lastWh = wh;
    CapturePlacement(window);
    const LONG fillMode = InterlockedCompareExchange(&g_desktopMirrorFill, 0, 0);
    RECT dst{};
    RECT scratchSrc{};
    bool useScratchSrc = false;
    bool needClear = true;
    LONG vx = 0, vy = 0, vw = (LONG)ww, vh = (LONG)wh;
    const bool haveVis = VisibleClientBox(window, ww, wh, vx, vy, vw, vh);
    const bool fitOk = ComputeMirrorDestination(desc.Width, desc.Height, desc.Width / 2, desc.Height,
                                                ww, wh, fillMode, haveVis, vx, vy, vw, vh,
                                                dst, scratchSrc, useScratchSrc, needClear);
    if (!fitOk) {
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
    const bool monoOrCutscene = !dup || g_scCropped || (InterlockedCompareExchange(&g_inCinematic, 0, 0) != 0);
    const UINT srcLeft = SourceLeft(desc.Width, mode, swap, g_scCropped, monoOrCutscene);
    Diag::lastSrcLeft = srcLeft;
    Diag::lastDst = dst;
    const RECT src{ (LONG)srcLeft, 0, (LONG)(srcLeft + desc.Width / 2), (LONG)desc.Height };
    hr = dev->StretchRect(back, &src, g_scratch, nullptr, D3DTEXF_LINEAR);
    if (FAILED(hr)) {
        Diag::lastHr = (LONG)hr;
        back->Release();
        InterlockedIncrement(&Diag::failStretch1); EmitMaybe(); return;
    }
    if (needClear) {
        hr = dev->ColorFill(back, nullptr, D3DCOLOR_XRGB(0, 0, 0));
        if (FAILED(hr)) {
            Diag::lastHr = (LONG)hr;
            back->Release();
            InterlockedIncrement(&Diag::failFill); EmitMaybe(); return;
        }
    }
    hr = dev->StretchRect(g_scratch, useScratchSrc ? &scratchSrc : nullptr, back, &dst, D3DTEXF_LINEAR);
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
