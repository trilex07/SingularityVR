#pragma once

#include <cstdint>

// Implemented in d3d9.cpp (after runtime Log). Call sites only need these declarations.
void PauseMenuDiagNoteCompose(int menuClass, bool offscreen, int compose, unsigned rtW,
                              unsigned rtH, unsigned sceneW, int canvasOrtho, float c5sx,
                              float c5ty);
void PauseMenuDiagNoteSubmit(bool dup, bool stereo, bool menuFlat, bool cropEyes, unsigned srcW,
                             unsigned srcH, int menuClass);
void PauseMenuDiagSampleSbsColumns(const uint8_t* bgra, unsigned width, unsigned height,
                                   unsigned pitchBytes);
