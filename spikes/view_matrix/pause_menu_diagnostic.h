#pragma once

#include "async_log.h"

extern volatile LONG g_pauseMenuDiag;
extern volatile LONG g_pauseMenuDiagSubmitLogged;
extern volatile LONG g_pauseMenuDiagSampleLogged;

inline void PauseMenuDiagNoteCompose(int menuClass, bool offscreen, int compose,
                                     unsigned rtW, unsigned rtH, unsigned sceneW,
                                     int canvasOrtho, float c5sx, float c5ty) {
    if (!InterlockedCompareExchange(&g_pauseMenuDiag, 0, 0) || menuClass != 3) return;
    static int seenOff = -1;
    static int seenCompose = -1;
    if (seenOff == (offscreen ? 1 : 0) && seenCompose == compose) return;
    seenOff = offscreen ? 1 : 0;
    seenCompose = compose;
    Log("pause-evidence compose: class=3 target=%s %ux%u sceneW=%u canvasOrtho=%d compose=%d"
        " c5 sx=%.4g ty=%.4g",
        offscreen ? "offscreen" : "backbuffer", rtW, rtH, sceneW, canvasOrtho, compose, c5sx,
        c5ty);
}

inline void PauseMenuDiagNoteSubmit(bool dup, bool stereo, bool menuFlat, bool cropEyes,
                                    unsigned srcW, unsigned srcH, int menuClass) {
    if (!InterlockedCompareExchange(&g_pauseMenuDiag, 0, 0) || menuClass != 3) return;
    if (InterlockedCompareExchange(&g_pauseMenuDiagSubmitLogged, 1, 0) != 0) return;
    Log("pause-evidence submit: class=3 dup=%d stereo=%d menuFlat=%d cropEyes=%d backbuffer=%ux%u"
        " (menuFlat=0 for class 3: each eye keeps its half of the SBS backbuffer)",
        dup ? 1 : 0, stereo ? 1 : 0, menuFlat ? 1 : 0, cropEyes ? 1 : 0, srcW, srcH);
}

inline void PauseMenuDiagSampleSbsColumns(const uint8_t* bgra, unsigned width, unsigned height,
                                          unsigned pitchBytes) {
    if (!InterlockedCompareExchange(&g_pauseMenuDiag, 0, 0) || !bgra || width < 8 || height < 8)
        return;
    if (InterlockedCompareExchange(&g_pauseMenuDiagSampleLogged, 1, 0) != 0) return;
    unsigned long colSum[4] = { 0, 0, 0, 0 };
    const unsigned colW = width / 4;
    const unsigned stepY = height / 64;
    if (stepY == 0) return;
    for (unsigned y = 0; y < height; y += stepY) {
        const uint8_t* row = bgra + y * pitchBytes;
        for (unsigned q = 0; q < 4; ++q) {
            const unsigned x0 = q * colW + colW / 4;
            const uint8_t* px = row + x0 * 4;
            colSum[q] += px[0] + px[1] + px[2];
        }
    }
    Log("pause-evidence sbs-columns: w=%u h=%u sampleLuma Q0=%lu Q1=%lu Q2=%lu Q3=%lu",
        width, height, colSum[0], colSum[1], colSum[2], colSum[3]);
}
