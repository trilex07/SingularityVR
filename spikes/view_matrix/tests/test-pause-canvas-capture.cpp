#include "../pause_canvas_backbuffer_trace.h"
#include "../pause_menu_mode.h"
#include <cstdio>

static int g_fails = 0;
static void Expect(bool cond, const char* name) {
    if (cond) { std::printf("PASS %s\n", name); return; }
    std::printf("FAIL %s\n", name);
    ++g_fails;
}

int main() {
    using namespace pausecanvas;
    Expect(ScaleBand(8e-5f, true) == Band::A, "band A scale threshold");
    Expect(ScaleBand(1.5e-5f, true) == Band::B, "band B scale threshold");
    Expect(ScaleBand(4e-5f, true) == Band::Mid, "mid band between A and B");
    Expect(ScaleBand(1e-3f, false) == Band::NotOrtho, "non-ortho excluded");

    using namespace pausemode;
    Expect(CapturesCanvasBackbuffer(CanvasCapture), "canvas mode flag");
    Expect(!CapturesCanvasBackbuffer(Legacy), "legacy does not capture canvas route");
    Expect(ShowsHeadCapture(CanvasCapture), "canvas presents on head plane");

    if (g_fails) return 1;
    std::puts("pause canvas capture: band classification and mode gates");
    return 0;
}
