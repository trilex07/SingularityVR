#pragma once
// Gate A (narrow): class-3 PauseBackbufferCanvasRoute batches only. No redirect.
#include "pause_menu_space.h"
#include <Windows.h>
#include <d3d9.h>
#include <intrin.h>
#include <cstring>

extern void Log(const char* fmt, ...);
extern volatile LONG g_gamePaused;

namespace pausecanvas {

enum class Band { None, A, B, Mid, NotOrtho };

inline Band ScaleBand(float sx, bool canvasOrtho) {
    if (!canvasOrtho) return Band::NotOrtho;
    if (sx > 6.0e-5f) return Band::A;
    if (sx < 2.2e-5f) return Band::B;
    return Band::Mid;
}

inline const char* BandTag(Band b) {
    switch (b) {
        case Band::A: return "A";
        case Band::B: return "B";
        case Band::Mid: return "mid";
        case Band::NotOrtho: return "not_ortho";
        default: return "-";
    }
}

struct Bounds {
    float minTx = 1e9f, maxTx = -1e9f, minTy = 1e9f, maxTy = -1e9f;
    unsigned count = 0;
    void Add(float tx, float ty) {
        if (tx < minTx) minTx = tx;
        if (tx > maxTx) maxTx = tx;
        if (ty < minTy) minTy = ty;
        if (ty > maxTy) maxTy = ty;
        ++count;
    }
};

struct CallerRec {
    void* raw = nullptr;
    char mod[64]{};
    uintptr_t off = 0;
};

inline void ResolveCaller(void* caller, CallerRec& out) {
    out.raw = caller;
    out.mod[0] = 0;
    out.off = 0;
    HMODULE mod = nullptr;
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            (LPCSTR)caller, &mod) ||
        !mod)
        return;
    char path[MAX_PATH]{};
    if (!GetModuleFileNameA(mod, path, MAX_PATH)) {
        strncpy(out.mod, "module", sizeof(out.mod) - 1);
    } else {
        const char* base = strrchr(path, '\\');
        const char* name = base ? base + 1 : path;
        strncpy(out.mod, name, sizeof(out.mod) - 1);
        out.mod[sizeof(out.mod) - 1] = 0;
    }
    out.off = (uintptr_t)caller - (uintptr_t)mod;
}

struct Txn {
    unsigned id = 0;
    unsigned frame = 0;
    unsigned long long token = 0;
    bool active = false;
    bool truncated = false;
    unsigned draws = 0;
    unsigned long long surf = 0;
    unsigned rtW = 0, rtH = 0;
    unsigned fmtLogged = 0;
    Bounds bandA, bandB, bandMid;
    unsigned countA = 0, countB = 0, countMid = 0, countNotOrtho = 0;
    bool panelOpen = false;
};

struct Session {
    volatile LONG enabled = 0;
    unsigned long long armedToken = 0;
    int framesRemaining = 0;
    unsigned txnSerial = 0;
    unsigned settledClosed = 0;
    Txn txn{};
    CallerRec callers[8]{};
    unsigned callerCount = 0;
    bool mappedExe = false;
};

inline Session& S() {
    static Session s{};
    return s;
}

inline void SetEnabled(bool on) { InterlockedExchange(&S().enabled, on ? 1 : 0); }

inline void NoteCaller(void* caller) {
    auto& s = S();
    for (unsigned i = 0; i < s.callerCount; ++i) {
        if (s.callers[i].raw == caller) return;
    }
    if (s.callerCount >= 8) return;
    ResolveCaller(caller, s.callers[s.callerCount++]);
}

inline void LogCallerMap(uintptr_t textLo, uintptr_t textHi) {
    auto& s = S();
    if (s.mappedExe) return;
    s.mappedExe = true;
    HMODULE d3d = GetModuleHandleA("d3d9.dll");
    Log("pause-canvas map: exe_text=%p..%p d3d9=%p (ReturnAddress is the HudStereoPair"
        " call site inside d3d9.dll, not RvUIGFxMovie_Pause)",
        (void*)textLo, (void*)textHi, (void*)d3d);
}

inline void Arm(unsigned long long token) {
    auto& s = S();
    if (!s.enabled) return;
    s.armedToken = token;
    s.framesRemaining = 360;
    s.callerCount = 0;
    s.mappedExe = false;
    Log("pause-canvas arm: token=%llX frames=360 scope=backbuffer_canvas_only", token);
}

inline void CloseTxn(const char* reason) {
    auto& s = S();
    Txn& t = s.txn;
    if (!t.active) return;
    const bool settled = t.countA > 0 && t.countB > 0;
    Log("pause-canvas txn=%u ev=end token=%llX reason=%s draws=%u surf=%p rtw=%u rth=%u"
        " bandA=%u bandB=%u mid=%u notOrtho=%u settled=%d"
        " boundsA_tx=[%.4f,%.4f] ty=[%.4f,%.4f]"
        " boundsB_tx=[%.4f,%.4f] ty=[%.4f,%.4f]"
        " boundsMid_tx=[%.4f,%.4f] ty=[%.4f,%.4f]",
        t.id, t.token, reason, t.draws, (void*)t.surf, t.rtW, t.rtH, t.countA, t.countB,
        t.countMid, t.countNotOrtho, settled ? 1 : 0,
        t.bandA.count ? t.bandA.minTx : 0.f, t.bandA.count ? t.bandA.maxTx : 0.f,
        t.bandA.count ? t.bandA.minTy : 0.f, t.bandA.count ? t.bandA.maxTy : 0.f,
        t.bandB.count ? t.bandB.minTx : 0.f, t.bandB.count ? t.bandB.maxTx : 0.f,
        t.bandB.count ? t.bandB.minTy : 0.f, t.bandB.count ? t.bandB.maxTy : 0.f,
        t.bandMid.count ? t.bandMid.minTx : 0.f, t.bandMid.count ? t.bandMid.maxTx : 0.f,
        t.bandMid.count ? t.bandMid.minTy : 0.f, t.bandMid.count ? t.bandMid.maxTy : 0.f);
    t.active = false;
    if (settled && reason && !strcmp(reason, "complete") && !t.truncated) ++s.settledClosed;
}

inline void EndFrame(unsigned frame, bool paused, int menuClass) {
    auto& s = S();
    if (!s.enabled) return;
    if (s.txn.active) {
        const char* reason = "complete";
        if (s.txn.truncated) reason = "truncated";
        CloseTxn(reason);
    }
    if (paused && menuClass == 3 && s.framesRemaining > 0 && s.armedToken) {
        --s.framesRemaining;
        if (s.framesRemaining == 0) {
            Log("pause-canvas capture exhausted token=%llX settled=%u", s.armedToken,
                s.settledClosed);
            for (unsigned i = 0; i < s.callerCount; ++i) {
                Log("pause-canvas caller: %s+0x%llX raw=%p", s.callers[i].mod,
                    (unsigned long long)s.callers[i].off, s.callers[i].raw);
            }
        }
    }
    (void)frame;
}

inline const char* UnknownWhy(Band band, bool canvasOrtho, uintptr_t tex, float sx) {
    if (!canvasOrtho) return "not_pause_canvas_ortho";
    if (band == Band::Mid && !tex) return "mid_scale_no_stage0_texture";
    if (band == Band::Mid && tex) return "mid_scale_distinct_texture";
    if (band == Band::NotOrtho) return "matrix_not_canvas";
    (void)sx;
    return "unclassified";
}

inline void Record(IDirect3DDevice9* dev, const float c5[16], unsigned frame,
                   unsigned long long token, bool panelOpen, void* caller, UINT stride,
                   UINT primCount, uintptr_t textLo, uintptr_t textHi) {
    auto& s = S();
    if (!s.enabled || !InterlockedCompareExchange(&g_gamePaused, 0, 0) || !s.armedToken ||
        token != s.armedToken || s.framesRemaining <= 0)
        return;
    LogCallerMap(textLo, textHi);
    NoteCaller(caller);

    if (!s.txn.active) {
        ++s.txnSerial;
        s.txn = {};
        s.txn.id = s.txnSerial;
        s.txn.frame = frame;
        s.txn.token = token;
        s.txn.active = true;
        s.txn.panelOpen = panelOpen;
        Log("pause-canvas txn=%u ev=begin token=%llX scope=backbuffer_canvas panel=%d frame=%u",
            s.txn.id, token, panelOpen ? 1 : 0, frame);
    }

    Txn& t = s.txn;
    if (t.draws >= 512) {
        t.truncated = true;
        return;
    }

    IDirect3DSurface9* rt = nullptr;
    dev->GetRenderTarget(0, &rt);
    const uintptr_t surf = (uintptr_t)rt;
    unsigned w = 0, h = 0;
    D3DFORMAT fmt = D3DFMT_UNKNOWN;
    if (rt) {
        D3DSURFACE_DESC d{};
        rt->GetDesc(&d);
        w = d.Width;
        h = d.Height;
        fmt = d.Format;
        rt->Release();
    }
    if (!t.surf) {
        t.surf = surf;
        t.rtW = w;
        t.rtH = h;
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

    const bool ortho = pausemenuspace::IsPauseCanvasOrtho(c5);
    const float sx = c5[0], sy = c5[5], tx = c5[12], ty = c5[13];
    const Band band = ScaleBand(sx, ortho);

    if (band == Band::A) {
        ++t.countA;
        t.bandA.Add(tx, ty);
    } else if (band == Band::B) {
        ++t.countB;
        t.bandB.Add(tx, ty);
    } else if (band == Band::Mid) {
        ++t.countMid;
        t.bandMid.Add(tx, ty);
    } else {
        ++t.countNotOrtho;
    }

    const char* why = (band == Band::Mid || band == Band::NotOrtho) ? UnknownWhy(band, ortho, texId, sx) : "-";

    CallerRec cr{};
    ResolveCaller(caller, cr);

    Log("pause-canvas txn=%u ev=draw token=%llX surf=%p fmt=%u vp=%u,%u,%u,%u scissor=%u,%ld,%ld,%ld,%ld"
        " band=%s sx=%.4g sy=%.4g tx=%.4g ty=%.4g tex=%p stride=%u prim=%u caller=%s+0x%llX why=%s",
        t.id, token, (void*)surf, (unsigned)fmt, vp.X, vp.Y, vp.Width, vp.Height,
        scOn ? 1u : 0u, sc.left, sc.top, sc.right, sc.bottom, BandTag(band), sx, sy, tx, ty,
        (void*)texId, (unsigned)stride, (unsigned)primCount, cr.mod,
        (unsigned long long)cr.off, why);

    ++t.draws;
}

}  // namespace pausecanvas
