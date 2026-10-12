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
    Expect(Clamp(4) == FullSource, "high mode clamps to full source");
    Expect(RedirectsOrtho(HeadGfx) && !RedirectsOrtho(Legacy) && !RedirectsOrtho(FullGfxMono),
           "only head mode redirects ortho batches");
    Expect(StretchesScene(FullGfxMono) && !StretchesScene(HeadGfx),
           "only full-gfx mode stretches the scene target");
    Expect(BakesBackbuffer(FullSource) && !BakesBackbuffer(FullGfxMono),
           "only full-source mode rewrites both backbuffer halves");
    Expect(ShowsHeadCapture(HeadGfx) && ShowsHeadCapture(FullGfxMono) && !ShowsHeadCapture(Legacy) &&
               !ShowsHeadCapture(FullSource),
           "head presentation is modes 1 and 2");
    Expect(ShortName(Legacy)[0] == 'L' && ShortName(FullSource)[0] == 'F', "short names exist");
    if (g_fails) return 1;
    std::puts("pause menu modes: four distinct sources");
    return 0;
}
