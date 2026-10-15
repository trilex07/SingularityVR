#pragma once
// Experimental menu submission policy. Class 1 is the IIS boot/title
// backbuffer. Classes 2-4 stay on the stereo scene even if a menu is focused.

inline bool MenuSubmitForcesFlat(int menuClass) {
    return menuClass == 1;
}

// Opt-in: reuse the established identical-crop bridge for focused frontend,
// and for a real pause over native cinema. Not dialogue, HUD or gameplay pause.
inline bool CinemaMenuPauseOwnsPresentation(bool enabled, int menuClass,
                                           bool focus, bool cinematic, bool paused) {
    return enabled && menuClass == 3 && focus && cinematic && paused;
}
inline bool CinemaMenuIdenticalCrop(bool enabled, int menuClass,
                                   bool focus, bool cinematic, bool paused) {
    return enabled && focus && (menuClass == 2 || menuClass == 4 ||
        CinemaMenuPauseOwnsPresentation(enabled, menuClass, focus, cinematic, paused));
}

// 1 crop, 2 half-rect, 3 stereo full-rect, 4 mono.
// Half-rect matches the submit branch `dup && !cropEyes`, including when stereo also selects a swapchain.
inline int MenuSubmitKind(bool cropEyes, bool dup, bool stereo) {
    if (cropEyes) return 1;
    if (dup && !cropEyes) return 2;
    if (stereo) return 3;
    return 4;
}
