#pragma once
// Experimental menu submission policy. Class 1 is the IIS boot/title
// backbuffer. Classes 2-4 stay on the stereo scene even if a menu is focused.

inline bool MenuSubmitForcesFlat(int menuClass) {
    return menuClass == 1;
}

// 1 crop, 2 half-rect, 3 stereo full-rect, 4 mono.
// Half-rect matches the submit branch `dup && !cropEyes`, including when stereo also selects a swapchain.
inline int MenuSubmitKind(bool cropEyes, bool dup, bool stereo) {
    if (cropEyes) return 1;
    if (dup && !cropEyes) return 2;
    if (stereo) return 3;
    return 4;
}
