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
extern volatile LONG g_desktopMirrorWindow;
extern volatile LONG g_steamCaptureBackbuffer;
extern volatile LONG g_inCinematic;
extern bool g_scCropped;
extern bool g_xrRunning;
extern unsigned g_srcW;
extern unsigned g_srcH;

namespace desktop_mirror {

// Stops before any mutation of the submitted backbuffer unless every
// preparatory operation succeeded. Also used by fault-injection CPU tests.
enum class ComposeStage { CopyEye, ClearCandidate, FitCandidate, Commit, Done };
template<class Ops> inline ComposeStage ComposeTransactional(Ops& ops, bool clear) {
    if (!ops.CopyEye()) return ComposeStage::CopyEye;
    if (clear && !ops.ClearCandidate()) return ComposeStage::ClearCandidate;
    if (!ops.FitCandidate()) return ComposeStage::FitCandidate;
    if (!ops.Commit()) return ComposeStage::Commit;
    return ComposeStage::Done;
}

inline IDirect3DSurface9* g_scratch = nullptr;
inline IDirect3DSurface9* g_composed = nullptr;
inline UINT g_scratchW = 0;
inline UINT g_scratchH = 0;
inline D3DFORMAT g_scratchFormat = D3DFMT_UNKNOWN;

// Geometry must be in physical monitor pixels, even when the game's process
// is DPI-unaware. Never change process/window DPI awareness for UE3 or Steam.
class PhysicalPixelScope {
    using SetContext = HANDLE (WINAPI*)(HANDLE);
    SetContext set_ = nullptr;
    HANDLE prior_ = nullptr;
public:
    PhysicalPixelScope() {
        static const auto set = reinterpret_cast<SetContext>(GetProcAddress(
            GetModuleHandleW(L"user32.dll"), "SetThreadDpiAwarenessContext"));
        set_ = set;
        if (set_) prior_ = set_(reinterpret_cast<HANDLE>(static_cast<INT_PTR>(-4)));
    }
    ~PhysicalPixelScope() { if (set_ && prior_) set_(prior_); }
    PhysicalPixelScope(const PhysicalPixelScope&) = delete;
    PhysicalPixelScope& operator=(const PhysicalPixelScope&) = delete;
};

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
    if (g_composed) { g_composed->Release(); g_composed = nullptr; }
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
    if (visX == 0 && visY == 0 && UINT(visW) == ww && UINT(visH) == wh)
        return FitDestination(bw, bh, sw, sh, ww, wh, out);
    const double sx = double(bw) / ww, sy = double(bh) / wh;
    // Fit in CLIENT pixels first. Applying the Present scale here and again
    // below distorted the eye whenever window and SBS backbuffer differed.
    const double aspect = double(sw) / sh;
    double w = double(visW), h = w / aspect;
    if (h > double(visH)) { h = double(visH); w = h * aspect; }
    if (w < 1 || h < 1) return false;
    const double ox = double(visX) + (double(visW) - w) / 2;
    const double oy = double(visY) + (double(visH) - h) / 2;
    const LONG l = LONG(ox * sx + .5), t = LONG(oy * sy + .5);
    out = { l, t, l + LONG(w * sx + .5), t + LONG(h * sy + .5) };
    return out.right <= (LONG)bw && out.bottom <= (LONG)bh && out.left >= 0 && out.top >= 0;
}

// Work-area geometry, including frame/title bar. Pure CPU; monitor origins may
// be negative. The cap is CLIENT pixels, never a stereo render-target size.
inline bool WindowBox(const RECT& work, LONG frameW, LONG frameH, RECT& out) {
    const LONG aw = work.right - work.left, ah = work.bottom - work.top;
    if (frameW < 0 || frameH < 0 || aw <= frameW || ah <= frameH) return false;
    const LONG cw = (aw - frameW < 1920) ? aw - frameW : 1920;
    const LONG ch = (ah - frameH < 1080) ? ah - frameH : 1080;
    if (cw < 64 || ch < 64) return false;
    const LONG w = cw + frameW, h = ch + frameH;
    const LONG l = work.left + (aw - w) / 2, t = work.top + (ah - h) / 2;
    out = {l, t, l + w, t + h};
    return true;
}

inline void FitWindow(HWND window, bool windowed) {
    if (!window || !windowed || IsIconic(window)) return;
    // Window messages can be owned by a different thread. Never synchronously
    // wait for it from the render thread, activate it, or change fullscreen.
    static HWND lastWindow = nullptr;
    static HMONITOR lastMonitor = nullptr;
    static RECT lastWork{};
    static ULONGLONG retryAt = 0;
    const ULONGLONG now = GetTickCount64();
    if (now < retryAt) return;
    retryAt = now + 1000;
    HMONITOR mon = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{}; mi.cbSize = sizeof(mi);
    if (!mon || !GetMonitorInfoW(mon, &mi)) return;
    // Fit once per window/monitor/work-area change, not every frame: users can
    // subsequently resize or move the window without fighting an auto-maximizer.
    if (lastWindow == window && lastMonitor == mon && EqualRect(&mi.rcWork, &lastWork)) return;
    RECT wr{}, cr{}, target{};
    if (!GetWindowRect(window, &wr) || !GetClientRect(window, &cr)) return;
    const LONG fw = (wr.right - wr.left) - (cr.right - cr.left);
    const LONG fh = (wr.bottom - wr.top) - (cr.bottom - cr.top);
    if (!WindowBox(mi.rcWork, fw, fh, target)) return;
    if (!SetWindowPos(window, nullptr, target.left, target.top,
                      target.right-target.left, target.bottom-target.top,
                      SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS)) return;
    lastWindow = window; lastMonitor = mon; lastWork = mi.rcWork;
    Log("mirror-window: requested outer=(%ld,%ld,%ld,%ld) frame=%ldx%ld client-cap=1920x1080; render targets unchanged by mirror",
        target.left,target.top,target.right,target.bottom,fw,fh);
}

// The Steam overlay may sample RT0 at Present. It must see the *completed*
// backbuffer rather than a restored UE3 offscreen target. No shader/constants
// are edited. Restore exact COM bindings and viewport after the original Present.
// Failure to acquire/prepare the scope leaves the original Present path intact.
class PresentBackbufferScope {
    IDirect3DDevice9* dev_ = nullptr;
    IDirect3DSurface9* rt_[4]{};
    IDirect3DSurface9* ds_ = nullptr;
    D3DVIEWPORT9 vp_{};
    UINT count_ = 0;
    bool saved_ = false;
    static inline volatile LONG boundLogs_ = 0, failureLogs_ = 0;
    static void Failure(const char* phase, HRESULT hr) {
        if (InterlockedIncrement(&failureLogs_) <= 4)
            Log("steam-capture: %s failed hr=0x%08lX (bounded; Steam pixels NOT validated)", phase,(unsigned long)hr);
    }
public:
    PresentBackbufferScope(IDirect3DDevice9* dev, bool enable) {
        if (!dev || !enable) return;
        D3DCAPS9 caps{};
        HRESULT hr = dev->GetDeviceCaps(&caps);
        if (FAILED(hr) || !caps.NumSimultaneousRTs) { Failure("caps",hr); return; }
        count_ = caps.NumSimultaneousRTs < 4 ? caps.NumSimultaneousRTs : 4;
        hr=dev->GetViewport(&vp_);
        if (FAILED(hr)) { Failure("save viewport",hr); return; }
        for (UINT i=0; i<count_; ++i) {
            hr = dev->GetRenderTarget(i, &rt_[i]);
            if ((i == 0 && (FAILED(hr) || !rt_[0])) ||
                (FAILED(hr) && hr != D3DERR_NOTFOUND)) { Failure("save RT",hr); Release(); return; }
        }
        const HRESULT dhr = dev->GetDepthStencilSurface(&ds_);
        if (FAILED(dhr) && dhr != D3DERR_NOTFOUND) { Failure("save depth",dhr); Release(); return; }
        IDirect3DSurface9* back = nullptr;
        hr=dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back);
        if (FAILED(hr) || !back) { Failure("backbuffer",hr); Release(); return; }
        dev_ = dev; saved_ = true;
        bool ok = SUCCEEDED(hr=dev->SetDepthStencilSurface(nullptr));
        for (UINT i=1; i<count_ && ok; ++i) ok = SUCCEEDED(hr=dev->SetRenderTarget(i,nullptr));
        if (ok) ok = SUCCEEDED(hr=dev->SetRenderTarget(0,back));
        if (ok && InterlockedIncrement(&boundLogs_) <= 4)
            Log("steam-capture: Present RT0=%p -> completed mirror backbuffer=%p; Steam F12 pixels pending",rt_[0],back);
        back->Release();
        if (!ok) { Failure("bind",hr); Restore(); }
    }
    PresentBackbufferScope(const PresentBackbufferScope&) = delete;
    PresentBackbufferScope& operator=(const PresentBackbufferScope&) = delete;
    ~PresentBackbufferScope() { Restore(); Release(); }
    bool Active() const { return saved_; }
private:
    void Restore() {
        if (!saved_) return;
        HRESULT hr=dev_->SetDepthStencilSurface(nullptr);
        if (FAILED(hr)) Failure("restore detach depth",hr);
        for (UINT i=1; i<count_; ++i) {
            hr=dev_->SetRenderTarget(i,nullptr);if(FAILED(hr))Failure("restore detach MRT",hr);
        }
        hr=dev_->SetRenderTarget(0,rt_[0]);if(FAILED(hr))Failure("restore RT0",hr);
        for (UINT i=1; i<count_; ++i) {
            hr=dev_->SetRenderTarget(i,rt_[i]);if(FAILED(hr))Failure("restore MRT",hr);
        }
        hr=dev_->SetDepthStencilSurface(ds_);if(FAILED(hr))Failure("restore depth",hr);
        hr=dev_->SetViewport(&vp_);if(FAILED(hr))Failure("restore viewport",hr);
        saved_ = false;
    }
    void Release() {
        for (auto& r : rt_) if (r) { r->Release(); r=nullptr; }
        if (ds_) { ds_->Release(); ds_=nullptr; }
    }
};

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

// ---- Reset guard: keep the window fit display-only ----
//
// MAIN observed on the centurion branch that fitting the game window shrinks
// the UE3 backbuffer and drops VR resolution: the engine follows WM_SIZE with
// a Reset sized to the window. The window fit must stay display-only while
// the render stays at the SteamVR SBS size, so Reset requests that merely
// follow the fitted window are redirected to the trusted SBS size.
//
// Pure decision function (L1-testable). The trusted size is the MEASURED copy
// size (g_srcW/H), never the AutoRes cache: a refused -ResX must not be
// enforced. req==0 means "use the window client area" in D3D9, so it is
// resolved against the live client size before judging.
struct ResetDecision { UINT w = 0, h = 0; bool enforced = false; };
inline ResetDecision DecideResetSize(UINT reqW, UINT reqH,
                                     UINT trustW, UINT trustH,
                                     UINT clientW, UINT clientH,
                                     bool fitActive, bool dupActive) {
    const ResetDecision pass{ reqW, reqH, false };
    if (!fitActive || !dupActive || !trustW || !trustH) return pass;
    const UINT effW = reqW ? reqW : clientW;
    const UINT effH = reqH ? reqH : clientH;
    if (!effW || !effH) return pass;               // nothing to judge against
    if (effW == trustW && effH == trustH) return pass;
    if (effW >= trustW && effH >= trustH) return pass;  // explicit bigger size: user's own choice
    const bool followsWindow =
        (clientW && clientH && effW == clientW && effH == clientH);
    if (!followsWindow) return pass;               // smaller but not window-sized: explicit choice, keep
    const ResetDecision hold{ trustW, trustH, true };
    return hold;
}

// ---- Screenshot stamp: mirror pixels into RT0, persistently ----
//
// The transient PresentBackbufferScope (binding RT0 to the backbuffer DURING
// Present) was falsified on Steam: F12 stayed black with it active, so the
// overlay does not sample the binding. What survives a pre-hook or
// post-restore capture is PIXELS, so after a successful mirror composition the
// finished backbuffer is also copied into the engine's current RT0 surface
// when it is a same-size/format alias. No state is changed (StretchRect does
// not rebind), nothing is read back to the CPU, and a failed stamp never
// touches the already-committed mirror.
struct StampDiag {
    static inline volatile LONG same = 0;     // RT0 already the backbuffer
    static inline volatile LONG stamped = 0;
    static inline volatile LONG skipped = 0;  // capture off / size mismatch
    static inline volatile LONG failed = 0;
    static inline LONG lastHr = 0;
};
inline bool ShouldStampRt(const D3DSURFACE_DESC& back, const D3DSURFACE_DESC& rt) {
    return back.Width == rt.Width && back.Height == rt.Height &&
           back.Format == rt.Format &&
           back.MultiSampleType == D3DMULTISAMPLE_NONE &&
           rt.MultiSampleType == D3DMULTISAMPLE_NONE;
}
inline bool StampCaptureRt(IDirect3DDevice9* dev) {
    if (!dev || InterlockedCompareExchange(&g_steamCaptureBackbuffer, 0, 0) == 0) {
        InterlockedIncrement(&StampDiag::skipped);
        return false;
    }
    IDirect3DSurface9* rt0 = nullptr;
    if (FAILED(dev->GetRenderTarget(0, &rt0)) || !rt0) {
        InterlockedIncrement(&StampDiag::failed);
        return false;
    }
    IDirect3DSurface9* back = nullptr;
    if (FAILED(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back)) || !back) {
        rt0->Release();
        InterlockedIncrement(&StampDiag::failed);
        return false;
    }
    bool done = false;
    if (rt0 == back) {
        InterlockedIncrement(&StampDiag::same);
    } else {
        D3DSURFACE_DESC db{}, dr{};
        const HRESULT hb = back->GetDesc(&db), hr = rt0->GetDesc(&dr);
        if (SUCCEEDED(hb) && SUCCEEDED(hr) && ShouldStampRt(db, dr)) {
            const HRESULT hs = dev->StretchRect(back, nullptr, rt0, nullptr, D3DTEXF_NONE);
            if (SUCCEEDED(hs)) {
                InterlockedIncrement(&StampDiag::stamped);
                done = true;
                static LONG said = 0;
                if (!InterlockedExchange(&said, 1))
                    Log("steam-capture: stamped completed mirror %ux%u fmt=%d into RT0; Steam F12 pixels pending",
                        (unsigned)db.Width, (unsigned)db.Height, (int)db.Format);
            } else {
                StampDiag::lastHr = (LONG)hs;
                InterlockedIncrement(&StampDiag::failed);
            }
        } else {
            if (FAILED(hb)) StampDiag::lastHr = (LONG)hb;
            else if (FAILED(hr)) StampDiag::lastHr = (LONG)hr;
            InterlockedIncrement(&StampDiag::skipped);
            static LONG saidDim = 0;
            if (!InterlockedExchange(&saidDim, 1))
                Log("steam-capture: RT0 %ux%u fmt=%d vs backbuffer %ux%u fmt=%d - stamp skipped (binding scope still covers Present)",
                    SUCCEEDED(hr) ? (unsigned)dr.Width : 0, SUCCEEDED(hr) ? (unsigned)dr.Height : 0,
                    SUCCEEDED(hr) ? (int)dr.Format : 0,
                    SUCCEEDED(hb) ? (unsigned)db.Width : 0, SUCCEEDED(hb) ? (unsigned)db.Height : 0,
                    SUCCEEDED(hb) ? (int)db.Format : 0);
        }
    }
    back->Release();
    rt0->Release();
    return done;
}

inline bool Apply(IDirect3DDevice9* dev, const RECT* presentSrc,
                  const RECT* presentDst, HWND overrideWindow, bool cineCrop = false) {
    const bool fitWindow = InterlockedCompareExchange(&g_desktopMirrorWindow, 0, 0) != 0;
    const LONG mode = fitWindow ? 2 : InterlockedCompareExchange(&g_desktopMirrorEye, 0, 0);
    Diag::lastMode = mode;
    const LONG dup = InterlockedCompareExchange(&g_dupDraws, 0, 0);
    if (mode <= 0) { InterlockedIncrement(&Diag::skipMode); EmitMaybe(); return false; }
    if (!dup && !g_xrRunning) { InterlockedIncrement(&Diag::skipDup); EmitMaybe(); return false; }
    if (!dev) { InterlockedIncrement(&Diag::skipRect); EmitMaybe(); return false; }

    IDirect3DSurface9* back = nullptr;
    HRESULT hr = dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back);
    if (FAILED(hr) || !back) {
        Diag::lastHr = (LONG)hr;
        InterlockedIncrement(&Diag::failBack); EmitMaybe(); return false;
    }
    D3DSURFACE_DESC desc{};
    hr = back->GetDesc(&desc);
    if (FAILED(hr) || desc.Width < 2 || (desc.Width & 1) || !desc.Height) {
        Diag::lastHr = (LONG)hr;
        back->Release();
        InterlockedIncrement(&Diag::failDesc); EmitMaybe(); return false;
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
        InterlockedIncrement(&Diag::skipRect); EmitMaybe(); return false;
    }
    if (!g_xrRunning || !g_srcW || !g_srcH) {
        back->Release();
        InterlockedIncrement(&Diag::skipXr); EmitMaybe(); return false;
    }

    D3DDEVICE_CREATION_PARAMETERS creation{};
    D3DPRESENT_PARAMETERS params{};
    IDirect3DSwapChain9* chain = nullptr;
    HWND window = overrideWindow;
    if (SUCCEEDED(dev->GetSwapChain(0, &chain)) && chain) {
        if (SUCCEEDED(chain->GetPresentParameters(&params)) && !window) window = params.hDeviceWindow;
    }
    if (chain) chain->Release();
    if (!window && SUCCEEDED(dev->GetCreationParameters(&creation))) window = creation.hFocusWindow;
    RECT client{};
    UINT ww = desc.Width, wh = desc.Height;
    LONG vx = 0, vy = 0, vw = (LONG)ww, vh = (LONG)wh;
    bool haveVis;
    {
        PhysicalPixelScope pixels;
        if (fitWindow) FitWindow(window, params.Windowed != FALSE);
        if (window && GetClientRect(window, &client) && client.right > 0 && client.bottom > 0) {
            ww = UINT(client.right); wh = UINT(client.bottom);
        }
        haveVis = VisibleClientBox(window, ww, wh, vx, vy, vw, vh);
    }
    Diag::lastWw = ww; Diag::lastWh = wh;
    CapturePlacement(window);
    const LONG fillMode = fitWindow ? 0 : InterlockedCompareExchange(&g_desktopMirrorFill, 0, 0);
    RECT dst{};
    RECT scratchSrc{};
    bool useScratchSrc = false;
    bool needClear = true;
    const bool fitOk = ComputeMirrorDestination(desc.Width, desc.Height, desc.Width / 2, desc.Height,
                                                ww, wh, fillMode, haveVis, vx, vy, vw, vh,
                                                dst, scratchSrc, useScratchSrc, needClear);
    if (!fitOk) {
        back->Release();
        InterlockedIncrement(&Diag::failFit); EmitMaybe(); return false;
    }
    if (!g_scratch || !g_composed || g_scratchW != desc.Width / 2 || g_scratchH != desc.Height ||
        g_scratchFormat != desc.Format) {
        ReleaseScratch();
        hr = dev->CreateRenderTarget(desc.Width / 2, desc.Height, desc.Format, D3DMULTISAMPLE_NONE,
                                     0, FALSE, &g_scratch, nullptr);
        if (SUCCEEDED(hr)) hr = dev->CreateRenderTarget(desc.Width, desc.Height, desc.Format,
            D3DMULTISAMPLE_NONE, 0, FALSE, &g_composed, nullptr);
        if (FAILED(hr)) {
            ReleaseScratch();
            Diag::lastHr = (LONG)hr;
            back->Release();
            InterlockedIncrement(&Diag::failScratch); EmitMaybe(); return false;
        }
        g_scratchW = desc.Width / 2;
        g_scratchH = desc.Height;
        g_scratchFormat = desc.Format;
    }

    const bool swap = InterlockedCompareExchange(&g_swapEyes, 0, 0) != 0;
    // Match the actual XR crop, not merely the cinematic flag. A stereo
    // cinematic still has independent halves; composed cinema always uses 0.
    const UINT srcLeft = cineCrop ? 0 : SourceLeft(desc.Width, mode, swap, g_scCropped, !dup);
    Diag::lastSrcLeft = srcLeft;
    Diag::lastDst = dst;
    const RECT src{ (LONG)srcLeft, 0, (LONG)(srcLeft + desc.Width / 2), (LONG)desc.Height };
    struct Ops {
        IDirect3DDevice9* dev;
        IDirect3DSurface9* back;
        const RECT *src, *scratchSrc, *dst;
        HRESULT hr = S_OK;
        bool CopyEye() { return SUCCEEDED(hr=dev->StretchRect(back,src,g_scratch,nullptr,D3DTEXF_LINEAR)); }
        bool ClearCandidate() { return SUCCEEDED(hr=dev->ColorFill(g_composed,nullptr,D3DCOLOR_XRGB(0,0,0))); }
        bool FitCandidate() { return SUCCEEDED(hr=dev->StretchRect(g_scratch,scratchSrc,g_composed,dst,D3DTEXF_LINEAR)); }
        bool Commit() { return SUCCEEDED(hr=dev->StretchRect(g_composed,nullptr,back,nullptr,D3DTEXF_NONE)); }
    } ops{dev,back,&src,useScratchSrc ? &scratchSrc : nullptr,&dst};
    const ComposeStage stage = ComposeTransactional(ops,needClear);
    if (stage != ComposeStage::Done) {
        Diag::lastHr = (LONG)ops.hr;
        back->Release();
        if (stage == ComposeStage::CopyEye) InterlockedIncrement(&Diag::failStretch1);
        else if (stage == ComposeStage::ClearCandidate) InterlockedIncrement(&Diag::failFill);
        else InterlockedIncrement(&Diag::failStretch2);
        EmitMaybe(); return false;
    }
    InterlockedIncrement(&Diag::ok);
    Diag::lastHr = S_OK;
    back->Release();
    EmitMaybe();
    return true;
}

} // namespace desktop_mirror
