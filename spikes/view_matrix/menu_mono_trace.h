#pragma once
// Bounded logging for GFx menu render targets (class 2/3). One line per
// (menuClass, backbuffer, rtW x rtH) tuple per process.

#include "pause_menu_space.h"

namespace menumono {

inline void NoteDraw(int menuClass, bool backbuffer, unsigned rtW, unsigned rtH,
                     unsigned srcW, const float c5[16], bool perEyePath) {
    if (menuClass != 2 && menuClass != 3) return;
    static int seenClass = -1;
    static int seenBack = -1;
    static unsigned seenW = 0, seenH = 0;
    static bool seenPerEye = false;
    const bool canvas = pausemenuspace::IsPauseCanvasOrtho(c5);
    if (seenClass == menuClass && seenBack == (backbuffer ? 1 : 0) &&
        seenW == rtW && seenH == rtH && seenPerEye == perEyePath && !canvas)
        return;
    seenClass = menuClass;
    seenBack = backbuffer ? 1 : 0;
    seenW = rtW;
    seenH = rtH;
    seenPerEye = perEyePath;
    Log("menu-mono trace: class=%d target=%s %ux%u sceneW=%u canvasOrtho=%d perEye=%d "
        "c5 sx=%.4g ty=%.4g",
        menuClass, backbuffer ? "backbuffer" : "offscreen", rtW, rtH, srcW, canvas ? 1 : 0,
        perEyePath ? 1 : 0, c5[0], c5[13]);
}

}  // namespace menumono
