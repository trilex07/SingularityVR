// Offline test for CutsceneFlat mono-to-both-eyes composition geometry.
// Build: cl /nologo /EHsc /O2 /std:c++17 test-cutscene-mono.cpp
#include <cstdio>
#include "../cutscene_mono.h"

static int g_failures = 0;
#define CHECK(cond) do { \
    if (!(cond)) { ++g_failures; \
        std::printf("FAIL line %d: %s\n", __LINE__, #cond); } \
} while (0)

// 4096x2160 SBS backbuffer: full image must land aspect-fit in the LEFT half.
static void test_sbs_fit() {
    const CutsceneMonoFit f = CutsceneMonoComputeFit(4096, 2160, 0, 2048, 2160);
    CHECK(f.valid);
    CHECK(f.image.left == 0 && f.image.right == 2048);
    CHECK(f.image.top == 540 && f.image.bottom == 1620);
    CHECK(f.barCount == 2); // top + bottom letterbox, no side bars
    // Aspect preserved: 4096/2160 == 2048/1080.
    CHECK((f.image.right - f.image.left) * 2160 == 4096 * (f.image.bottom - f.image.top));
    // Bars cover exactly the half outside the image.
    int barPx = 0;
    for (int i = 0; i < f.barCount; ++i)
        barPx += (f.bars[i].right - f.bars[i].left) * (f.bars[i].bottom - f.bars[i].top);
    CHECK(barPx + (f.image.right - f.image.left) * (f.image.bottom - f.image.top) == 2048 * 2160);
}

// Odd widths must stay valid, contained, and aspect-close (within 2 px rows).
static void test_odd_dims() {
    const CutsceneMonoFit f = CutsceneMonoComputeFit(4095, 2160, 0, 2047, 2160);
    CHECK(f.valid);
    CHECK(f.image.left >= 0 && f.image.right <= 2047);
    CHECK(f.image.top >= 0 && f.image.bottom <= 2160);
    const int dw = f.image.right - f.image.left, dh = f.image.bottom - f.image.top;
    CHECK(dw > 0 && dh > 0);
    // |dw/dh - srcW/srcH| small: cross-multiplied, tolerance 2 rows/cols.
    long long a = (long long)dw * 2160, b = (long long)4095 * dh;
    long long tol = 2LL * (2160 + 4095);
    CHECK(a > b ? a - b < tol : b - a < tol);
}

static void test_degenerate() {
    CHECK(!CutsceneMonoComputeFit(0, 2160, 0, 2048, 2160).valid);
    CHECK(!CutsceneMonoComputeFit(4096, 0, 0, 2048, 2160).valid);
    CHECK(!CutsceneMonoComputeFit(4096, 2160, 0, 0, 2160).valid);
    CHECK(!CutsceneMonoComputeFit(4096, 2160, -1, 2048, 2160).valid);
}

static void test_predicate() {
    using M = CutsceneMonoMode;
    CHECK(CutsceneMonoShouldCompose(M::CutsceneFlat, true, false, true));
    CHECK(!CutsceneMonoShouldCompose(M::VideoFlat, true, false, true)); // Bink self-handles
    CHECK(!CutsceneMonoShouldCompose(M::Other, true, false, true));
    CHECK(!CutsceneMonoShouldCompose(M::CutsceneFlat, false, false, true)); // ALWAYS VR
    CHECK(!CutsceneMonoShouldCompose(M::CutsceneFlat, true, true, true));   // menu owns it
    CHECK(!CutsceneMonoShouldCompose(M::CutsceneFlat, true, false, false)); // flat desktop
}

static void test_camera_change_gate() {
    using C = CameraChangeCtx;
    // Matinee camera cut: same pawn, no menu -> keep CINE.
    CHECK((!CameraChangeClearsCine(C{ true, true, true, false })));
    // True level transition: pawn recreated -> clear.
    CHECK((CameraChangeClearsCine(C{ true, false, true, false })));
    // World gone (menu load, pawn null) -> clear.
    CHECK((CameraChangeClearsCine(C{ true, false, false, false })));
    CHECK((CameraChangeClearsCine(C{ true, true, false, false })));
    // Quit to menu with surviving pawn value -> clear.
    CHECK((CameraChangeClearsCine(C{ true, true, true, true })));
    // Nothing latched -> no-op regardless.
    CHECK((!CameraChangeClearsCine(C{ false, false, false, false })));
    CHECK((!CameraChangeClearsCine(C{ false, true, true, false })));
}

int main() {
    test_sbs_fit();
    test_odd_dims();
    test_degenerate();
    test_predicate();
    test_camera_change_gate();
    if (g_failures == 0) { std::printf("PASS: cutscene mono fit + predicate\n"); return 0; }
    std::printf("FAILED: %d checks\n", g_failures);
    return 1;
}
