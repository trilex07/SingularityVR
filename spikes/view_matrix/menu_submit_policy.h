#pragma once
// Experimental menu submission policy. Class 1 is the IIS boot/title
// backbuffer. Classes 2-4 stay on the stereo scene even if a menu is focused.

inline bool MenuSubmitForcesFlat(int menuClass) {
    return menuClass == 1;
}
