#pragma once
// Bounded in-game PauseComposeTrace logger. Diagnostic only; no redirect.
#include "pause_compose_trace.h"
#include "pause_menu_space.h"
#include <Windows.h>
#include <d3d9.h>
#include <intrin.h>

extern void Log(const char* fmt, ...);

namespace pausecompose {
namespace runtime {

constexpr unsigned kMaxDrawsPerTxn = 512;
constexpr unsigned kMaxClosedTxns = 16;
constexpr int kCaptureFrames = 360;

struct TxnLive {
    unsigned id = 0;
    unsigned frame = 0;
    unsigned long long token = 0;
    unsigned long long beginToken = 0;
    bool active = false;
    bool truncated = false;
    unsigned draws = 0;
    bool sawMain = false;
    bool sawObj = false;
    bool sawSelection = false;
    unsigned long long menuSurf = 0;
    bool haveSurf = false;
    bool panelOpen = false;
    State state = State::Other;
};

struct CaptureSession {
    volatile LONG enabled = 0;
    unsigned long long armedToken = 0;
    int framesRemaining = 0;
    unsigned txnSerial = 0;
    unsigned closedComplete = 0;
    TxnLive txn{};
};

inline CaptureSession& Caps() {
    static CaptureSession c{};
    return c;
}

inline bool CaptureWindowOpen(const CaptureSession& c) {
    return c.framesRemaining > 0 && c.armedToken != 0;
}

inline const char* StateTag(State s) {
    switch (s) {
        case State::Root: return "root";
        case State::Selection: return "selection";
        case State::Objectives: return "objectives";
        default: return "other";
    }
}

inline const char* GroupTag(Group g) {
    switch (g) {
        case Group::Main: return "main";
        case Group::Objectives: return "objectives";
        case Group::Selection: return "selection";
        case Group::Cursor: return "cursor";
        case Group::Panel: return "panel";
        case Group::World: return "world";
        case Group::Overlay: return "overlay";
        default: return "unknown";
    }
}

// Measured 96390ab canvas ortho bands (bc83af6). Texture id disambiguates highlights.
inline Group ClassifyProducer(const float c5[16], uintptr_t texId) {
    if (!pausemenuspace::IsPauseCanvasOrtho(c5)) return Group::World;
    const float sx = c5[0];
    const float ty = c5[13];
    if (sx < 2.2e-5f && ty < 0.57f) return Group::Objectives;
    if (sx > 6.0e-5f) return Group::Main;
    if (texId) return Group::Selection;
    return Group::Unknown;
}

inline void SetEnabled(bool on) { InterlockedExchange(&Caps().enabled, on ? 1 : 0); }

inline void Arm(unsigned long long token) {
    auto& c = Caps();
    if (!c.enabled) return;
    c.armedToken = token;
    c.framesRemaining = kCaptureFrames;
    Log("pause-compose arm: token=%llX captureFrames=%d", token, kCaptureFrames);
}

inline void CloseTxn(const char* reason) {
    auto& c = Caps();
    TxnLive& t = c.txn;
    if (!t.active) return;
    const bool complete = reason && !strcmp(reason, "complete") && !t.truncated &&
                          t.token == t.beginToken;
    Log("pause-compose txn=%u ev=end token=%llX reason=%s draws=%u truncated=%d main=%d"
        " objectives=%d selection=%d state=%s",
        t.id, t.beginToken, reason ? reason : "fallback", t.draws, t.truncated ? 1 : 0,
        t.sawMain ? 1 : 0, t.sawObj ? 1 : 0, t.sawSelection ? 1 : 0, StateTag(t.state));
    t.active = false;
    if (complete) ++c.closedComplete;
}

inline void EndPresentFrame(unsigned frame, bool gamePaused, int menuClass) {
    auto& c = Caps();
    if (!c.enabled) return;
    if (c.txn.active) {
        const char* reason = "complete";
        if (c.txn.truncated) reason = "truncated";
        else if (c.txn.token != c.txn.beginToken) reason = "stale";
        CloseTxn(reason);
    }
    if (gamePaused && menuClass == 3 && CaptureWindowOpen(c)) {
        --c.framesRemaining;
        if (c.framesRemaining == 0)
            Log("pause-compose capture window exhausted token=%llX closed=%u",
                c.armedToken, c.closedComplete);
    }
    (void)frame;
}

inline void MaybeBegin(unsigned frame, unsigned long long token, bool panelOpen) {
    auto& c = Caps();
    if (!CaptureWindowOpen(c) || token != c.armedToken) return;
    if (c.txn.active) return;
    if (c.closedComplete >= kMaxClosedTxns) return;
    ++c.txnSerial;
    c.txn = {};
    c.txn.id = c.txnSerial;
    c.txn.frame = frame;
    c.txn.token = token;
    c.txn.beginToken = token;
    c.txn.active = true;
    c.txn.panelOpen = panelOpen;
    c.txn.state = State::Root;
    Log("pause-compose txn=%u ev=begin token=%llX owner=3 panel=%d state=root frame=%u",
        c.txn.id, token, panelOpen ? 1 : 0, frame);
}

inline void RecordBatch(IDirect3DDevice9* dev, const float c5[16], int menuClass, bool gamePaused,
                        bool backbuffer, int compose, unsigned frame, unsigned long long token,
                        bool panelOpen, void* caller) {
    auto& c = Caps();
    if (!c.enabled || menuClass != 3 || !gamePaused) return;
    if (!CaptureWindowOpen(c) || token != c.armedToken) return;
    if (c.closedComplete >= kMaxClosedTxns) return;

    MaybeBegin(frame, token, panelOpen);
    if (!c.txn.active) return;
    if (token != c.txn.beginToken) {
        CloseTxn("stale");
        return;
    }

    TxnLive& t = c.txn;
    if (t.draws >= kMaxDrawsPerTxn) {
        t.truncated = true;
        return;
    }

    IDirect3DSurface9* rt = nullptr;
    dev->GetRenderTarget(0, &rt);
    const uintptr_t surf = (uintptr_t)rt;
    unsigned rtw = 0;
    if (rt) {
        D3DSURFACE_DESC d{};
        rt->GetDesc(&d);
        rtw = d.Width;
        rt->Release();
    }
    D3DVIEWPORT9 vp{};
    dev->GetViewport(&vp);
    RECT sc{};
    DWORD scOn = FALSE;
    dev->GetScissorRect(&sc);
    dev->GetRenderState(D3DRS_SCISSORTESTENABLE, &scOn);

    IDirect3DBaseTexture9* tex = nullptr;
    dev->GetTexture(0, &tex);
    const uintptr_t texId = (uintptr_t)tex;
    if (tex) tex->Release();

    Group group = Group::World;
    int owner = 2;
    if (backbuffer && compose == 1 && pausemenuspace::IsPauseCanvasOrtho(c5)) {
        group = ClassifyProducer(c5, texId);
        owner = 3;
    } else if (!backbuffer && menuClass == 3) {
        group = pausemenuspace::IsPauseCanvasOrtho(c5) ? Group::Unknown : Group::World;
        owner = 3;
    } else if (pausemenuspace::IsPauseCanvasOrtho(c5)) {
        group = ClassifyProducer(c5, texId);
    }

    if (group == Group::Main) t.sawMain = true;
    if (group == Group::Objectives) t.sawObj = true;
    if (group == Group::Selection) t.sawSelection = true;
    if (t.sawObj) t.state = State::Objectives;
    else if (t.sawSelection) t.state = State::Selection;
    else if (t.sawMain) t.state = State::Root;

    if (IsMenuGroup(group)) {
        if (!t.haveSurf) {
            t.menuSurf = surf;
            t.haveSurf = true;
        }
    }

    Log("pause-compose txn=%u ev=draw token=%llX surf=%p group=%s owner=%d vp=%u,%u,%u,%u rtw=%u"
        " scissor=%u,%ld,%ld,%ld,%ld caller=%p prod=c5,tex",
        t.id, token, (void*)surf, GroupTag(group), owner, vp.X, vp.Y, vp.Width, vp.Height, rtw,
        scOn ? 1u : 0u, sc.left, sc.top, sc.right, sc.bottom, caller);

    ++t.draws;
}

}  // namespace runtime
}  // namespace pausecompose
