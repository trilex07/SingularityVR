#pragma once
// Pause GFx (class 3) often reaches HudStereoPair on the scene-sized colour target
// without ever binding the private 1280x720 sink the main menu uses. Redirect those
// ortho batches into a pooled GFx-sized RT so the existing capture/provider path can
// draw the same texture head-relative. Off by default until headset acceptance.
namespace pausehead {
constexpr unsigned kPoolWidth = 1280;
constexpr unsigned kPoolHeight = 720;

inline bool IniEnabled(int iniValue) { return iniValue != 0; }

inline bool RedirectOrthoDraws(int menuClass, bool iniEnabled, bool vrMode, bool uiMenu,
                               bool menuOverlay, bool panelOpen) {
    if (!iniEnabled || !vrMode || !uiMenu || !menuOverlay || panelOpen) return false;
    return menuClass == 3;
}
}  // namespace pausehead
