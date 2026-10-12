#include "../pause_menu_mode.h"
#include <cstdio>

static int g_fails = 0;
static void Expect(bool cond, const char* name) {
    if (cond) { std::printf("PASS %s\n", name); return; }
    std::printf("FAIL %s\n", name);
    ++g_fails;
}

int main() {
    using namespace pausemode;
    Expect(Clamp(-1) == Legacy, "negative mode clamps to legacy");
    Expect(Clamp(4) == CanvasCapture, "high mode clamps to canvas capture");
    Expect(RedirectsOrtho(HeadGfx) && !RedirectsOrtho(Legacy) && !RedirectsOrtho(FullGfxMono),
           "only head mode redirects ortho batches");
    Expect(StretchesScene(FullGfxMono) && !StretchesScene(HeadGfx),
           "only full-gfx mode stretches the scene target");
    Expect(!BakesBackbuffer(CanvasCapture) && !BakesBackbuffer(FullGfxMono),
           "no mode rewrites both backbuffer halves at present");
    Expect(CapturesCanvasBackbuffer(CanvasCapture) && !CapturesCanvasBackbuffer(Legacy),
           "only canvas mode mirrors backbuffer route");
    Expect(ShowsHeadCapture(HeadGfx) && ShowsHeadCapture(FullGfxMono) &&
               ShowsHeadCapture(CanvasCapture) && !ShowsHeadCapture(Legacy),
           "head presentation is modes 1-3");
    Expect(ShortName(Legacy)[0] == 'L' && ShortName(CanvasCapture)[0] == 'C', "short names exist");
    if (g_fails) return 1;
    std::puts("pause menu modes: four distinct sources");
    return 0;
}
