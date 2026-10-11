#pragma once
// Pause GFx (class 3) ortho batches on the diegetic menu-mono path can redirect into a
// pooled 1280x720 RT when PauseHeadMenu=1. Default off until headset acceptance.
#include <cmath>
#include <cstdint>

namespace pausehead {
constexpr unsigned kPoolWidth = 1280;
constexpr unsigned kPoolHeight = 720;
// Interactive pause GFx sits in the panel; corner/status HUD uses clip positions near the edges.
constexpr float kPauseGfxClipEdge = 0.88f;

inline bool IniEnabled(int iniValue) { return iniValue != 0; }

inline bool RedirectOrthoDraws(int menuClass, bool iniEnabled, bool vrMode, bool uiMenu,
                               bool menuOverlay, bool panelOpen) {
    if (!iniEnabled || !vrMode || !uiMenu || !menuOverlay || panelOpen) return false;
    return menuClass == 3;
}

inline bool GfxOrthoClipLooksLikePauseMenu(float clipX, float clipY) {
    return std::fabs(clipX) <= kPauseGfxClipEdge && std::fabs(clipY) <= kPauseGfxClipEdge;
}

// Legacy class-2/3 menu-mono branch, native GFx focus, paused, plus centered clip (not corner HUD).
inline bool GfxPauseMenuMonoBatch(int menuClass, bool menuLayout, bool nativeReady, bool gamePaused,
                                  bool noteOpen, bool rtIsScene, bool rtIsBackbuffer, float clipX,
                                  float clipY, bool iniEnabled, bool vrMode, bool menuOverlay,
                                  bool panelOpen) {
    if (menuClass != 3 || rtIsBackbuffer || !rtIsScene) return false;
    if (!menuLayout || !nativeReady || !gamePaused || noteOpen) return false;
    if (!GfxOrthoClipLooksLikePauseMenu(clipX, clipY)) return false;
    return RedirectOrthoDraws(menuClass, iniEnabled, vrMode, menuLayout, menuOverlay, panelOpen);
}

inline bool ShouldClearPool(unsigned clearedForFrame, unsigned frameKey) {
    return clearedForFrame != frameKey;
}

enum class BindStep : int {
    Ok = 0,
    PoolUnavailable,
    SaveStateFailed,
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

inline bool MustRestoreAfterBindFailure(BindStep step) {
    switch (step) {
        case BindStep::SaveStateFailed:
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

// Mirrors D3D9 fields saved around the pooled pass (L1 policy tests only).
struct DeviceStateFields {
    bool haveRt = false;
    bool haveVp = false;
    bool haveScissor = false;
    bool haveDepth = false;
    bool poolBound = false;
};

inline DeviceStateFields PlanDeviceRestore(BindStep step, bool savedState) {
    DeviceStateFields f{};
    f.haveRt = savedState;
    f.haveVp = savedState;
    f.haveScissor = savedState;
    f.haveDepth = savedState;
    f.poolBound = step == BindStep::Ok;
    return f;
}

inline bool MustRestoreDeviceState(const DeviceStateFields& f) {
    return f.haveRt || f.haveVp || f.haveScissor || f.haveDepth || f.poolBound;
}
}  // namespace pausehead
