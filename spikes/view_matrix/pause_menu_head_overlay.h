#pragma once
// Pause GFx (class 3) ortho batches that reach the diegetic menu-mono path can be redirected
// into a pooled GFx-sized RT when PauseHeadMenu=1. Off by default until headset acceptance.
#include <cstdint>

namespace pausehead {
constexpr unsigned kPoolWidth = 1280;
constexpr unsigned kPoolHeight = 720;

inline bool IniEnabled(int iniValue) { return iniValue != 0; }

inline bool RedirectOrthoDraws(int menuClass, bool iniEnabled, bool vrMode, bool uiMenu,
                               bool menuOverlay, bool panelOpen) {
    if (!iniEnabled || !vrMode || !uiMenu || !menuOverlay || panelOpen) return false;
    return menuClass == 3;
}

// Same eligibility as the legacy class-2/3 menu-mono branch, plus native GFx focus and pause.
inline bool GfxPauseMenuMonoBatch(int menuClass, bool menuLayout, bool nativeReady, bool gamePaused,
                                  bool rtIsScene, bool rtIsBackbuffer, bool iniEnabled, bool vrMode,
                                  bool menuOverlay, bool panelOpen) {
    if (menuClass != 3 || rtIsBackbuffer || !rtIsScene) return false;
    if (!menuLayout || !nativeReady || !gamePaused) return false;
    return RedirectOrthoDraws(menuClass, iniEnabled, vrMode, menuLayout, menuOverlay, panelOpen);
}

inline bool ShouldClearPool(unsigned clearedForFrame, unsigned frameKey) {
    return clearedForFrame != frameKey;
}

enum class BindStep : int {
    Ok = 0,
    PoolUnavailable,
    GetRenderTargetFailed,
    GetViewportFailed,
    SetRenderTargetFailed,
    SetViewportFailed,
    ClearFailed,
};

struct RedirectPlan {
    bool attemptRedirect = false;
    bool clearPool = false;
    unsigned frameKey = 0;
};

inline RedirectPlan PlanGfxRedirect(unsigned clearedForFrame, unsigned frameKey, bool eligible,
                                    bool poolReady) {
    RedirectPlan p{};
    p.attemptRedirect = eligible && poolReady;
    p.frameKey = frameKey;
    if (p.attemptRedirect) p.clearPool = ShouldClearPool(clearedForFrame, frameKey);
    return p;
}

// After a failed bind/clear, caller must not draw to the pool and should restore D3D9 state.
inline bool MustRestoreAfterBindFailure(BindStep step) {
    switch (step) {
        case BindStep::GetViewportFailed:
        case BindStep::SetRenderTargetFailed:
        case BindStep::SetViewportFailed:
        case BindStep::ClearFailed:
            return true;
        default:
            return false;
    }
}

inline bool CaptureAfterDrawAllowed(BindStep bindStep, bool drawSucceeded) {
    return bindStep == BindStep::Ok && drawSucceeded;
}
}  // namespace pausehead
