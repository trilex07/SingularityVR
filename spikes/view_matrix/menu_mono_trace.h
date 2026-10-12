#pragma once
// Bounded logging for GFx menu render targets (class 2/3). One line per
// (menuClass, backbuffer, rtW x rtH) tuple per process.

#include "pause_menu_space.h"

namespace menumono {

// compose: 0=single full-width, 1=canvas c5 per-eye
inline void NoteDraw(int menuClass, bool backbuffer, unsigned rtW, unsigned rtH,
                     unsigned srcW, const float c5[16], int compose) {
    if (menuClass != 2 && menuClass != 3) return;
    static int seenClass = -1;
    static int seenBack = -1;
    static unsigned seenW = 0, seenH = 0;
    static int seenCompose = -1;
    const bool canvas = pausemenuspace::IsPauseCanvasOrtho(c5);
    if (seenClass == menuClass && seenBack == (backbuffer ? 1 : 0) &&
        seenW == rtW && seenH == rtH && seenCompose == compose && !canvas)
        return;
    seenClass = menuClass;
    seenBack = backbuffer ? 1 : 0;
    seenW = rtW;
    seenH = rtH;
    seenCompose = compose;
    Log("menu-mono trace: class=%d target=%s %ux%u sceneW=%u canvasOrtho=%d compose=%d "
        "c5 sx=%.4g ty=%.4g",
        menuClass, backbuffer ? "backbuffer" : "offscreen", rtW, rtH, srcW, canvas ? 1 : 0,
        compose, c5[0], c5[13]);
}

}  // namespace menumono
