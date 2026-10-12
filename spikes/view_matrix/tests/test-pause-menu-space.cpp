#include "../pause_menu_space.h"
#include <cstdio>

static int g_fails = 0;

static void Expect(bool cond, const char* name) {
    if (cond) {
        std::printf("PASS %s\n", name);
        return;
    }
    std::printf("FAIL %s\n", name);
    ++g_fails;
}

// Measured pause font draw, view_matrix_2026-10-12_09-34-45.log MDG 113.
// sx = 1/12800, sy = -1/7200, translation parks the glyph on the left of the
// full frame. A second UI ortho in that sample sits right of the seam.
static void MeasuredFont(float m[16]) {
    for (int i = 0; i < 16; ++i) m[i] = 0.0f;
    m[0] = 7.812e-05f;
    m[5] = -0.0001389f;
    m[10] = 1.0f;
    m[12] = -0.8721f;
    m[13] = 0.4301f;
    m[15] = 1.0f;
}

int main() {
    float font[16];
    MeasuredFont(font);
    Expect(pausemenuspace::IsPauseCanvasOrtho(font),
           "measured pause font matrix is a canvas ortho");

    float right[16];
    MeasuredFont(right);
    right[0] = 6.152e-06f;
    right[5] = -1.094e-05f;
    right[12] = 0.3326f;
    right[13] = 0.1809f;
    Expect(pausemenuspace::IsPauseCanvasOrtho(right),
           "the second pause UI ortho is recognised");

    float world[16] = {};
    world[0] = world[5] = world[10] = world[15] = 1.0f;
    world[12] = -17710.0f;
    world[13] = 18440.0f;
    Expect(!pausemenuspace::IsPauseCanvasOrtho(world),
           "a camera-scale world matrix is not a pause canvas");

    float perspective[16] = {};
    perspective[0] = 0.001f;
    perspective[5] = -0.001f;
    perspective[11] = 1.0f;
    perspective[15] = 0.0f;
    Expect(!pausemenuspace::IsPauseCanvasOrtho(perspective),
           "a perspective matrix is not a pause canvas");

    const float inset = 0.84f;
    float lx = 0, ly = 0, rx = 0, ry = 0;
    pausemenuspace::HudEyeClip(font[12], font[13], inset, 0, lx, ly);
    pausemenuspace::HudEyeClip(font[12], font[13], inset, 1, rx, ry);
    Expect(lx > -1.0f && lx < 0.0f && rx > 0.0f && rx < 1.0f,
           "left-side pause text lands inside both eyes");
    Expect(ly > -1.0f && ly < 1.0f && ry > -1.0f && ry < 1.0f,
           "pause text keeps a readable vertical position");

    float lx2 = 0, rx2 = 0, y2 = 0;
    pausemenuspace::HudEyeClip(right[12], right[13], inset, 0, lx2, y2);
    pausemenuspace::HudEyeClip(right[12], right[13], inset, 1, rx2, y2);
    Expect(lx2 > -1.0f && lx2 < 0.0f && rx2 > 0.0f && rx2 < 1.0f,
           "right-of-seam pause text also lands inside both eyes");

    float canvas[16];
    MeasuredFont(canvas);
    Expect(pausemenuspace::DiegeticMenuCanvas(canvas, 5264, 5264),
           "scene-sized GFx target uses per-eye canvas composition");
    Expect(!pausemenuspace::DiegeticMenuCanvas(canvas, 1280, 5264),
           "small monitor RT keeps single draw");

    float bbCanvas[16];
    MeasuredFont(bbCanvas);
    Expect(pausemenuspace::PauseBackbufferCanvasRoute(3, true, bbCanvas),
           "in-game pause canvas ortho on backbuffer uses per-eye copy");
    Expect(!pausemenuspace::PauseBackbufferCanvasRoute(3, false, bbCanvas),
           "offscreen pause canvas is not the backbuffer route");

    Expect(pausemenuspace::PauseOffscreenMonoStage(3, 5264, 5264),
           "in-game pause on scene-sized RT uses mono stage viewport");
    Expect(!pausemenuspace::PauseOffscreenMonoStage(2, 5264, 5264),
           "main menu does not take pause mono stage");
    Expect(!pausemenuspace::PauseOffscreenMonoStage(3, 512, 5264),
           "sub-rect monitor RT is not pause mono stage");
    return g_fails == 0 ? 0 : 1;
}
