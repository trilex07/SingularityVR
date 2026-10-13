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

static void test_smaller_cinema_and_projection() {
    const auto legacy=CutsceneMonoComputeFit(5264,2684,0,2632,2684);
    const auto small=CutsceneMonoComputeFit(5264,2684,0,2632,2684,85);
    CHECK(small.valid && small.barCount==4);
    const int w=small.image.right-small.image.left, h=small.image.bottom-small.image.top;
    CHECK(w==2237 && h==1140);
    CHECK(small.image.left>0 && small.image.right<2632);
    CHECK(w*2684LL-h*5264LL<2LL*(5264+2684));
    CHECK(w<legacy.image.right-legacy.image.left);
    CHECK(CutsceneMonoComputeFit(5264,2684,0,2632,2684,200).image.right==legacy.image.right);
    using M=CutsceneMonoMode;
    const float eyeX=1.3f, y=1.32f;
    const float fullX=CutsceneMonoRenderTanX(M::CutsceneFlat,true,true,eyeX);
    CHECK(fullX==2.f*eyeX);
    // Same unit geometry, full viewport twice as wide: corrected tangent cancels
    // the old 2x horizontal stretch, without changing the submitted eye frustum.
    CHECK(5264.f/fullX == 2632.f/eyeX);
    CHECK((5264.f/fullX)/(2684.f/y) < (5264.f/eyeX)/(2684.f/y));
    CHECK(CutsceneMonoRenderTanX(M::VideoFlat,true,true,eyeX)==eyeX);
    CHECK(CutsceneMonoRenderTanX(M::Other,true,true,eyeX)==eyeX);
    CHECK(CutsceneMonoRenderTanX(M::CutsceneFlat,false,true,eyeX)==eyeX);
    CHECK(CutsceneMonoRenderTanX(M::CutsceneFlat,true,false,eyeX)==eyeX);
    CHECK(CutsceneHideTrackedForeground(M::CutsceneFlat,true,true,false,true,false,true,true));
    CHECK(!CutsceneHideTrackedForeground(M::Other,true,true,false,true,false,true,true));
    CHECK(!CutsceneHideTrackedForeground(M::VideoFlat,true,true,false,true,false,true,true));
    CHECK(!CutsceneHideTrackedForeground(M::CutsceneFlat,true,true,true,true,false,true,true));
    CHECK(!CutsceneHideTrackedForeground(M::CutsceneFlat,true,true,false,true,true,true,true));
    CHECK(!CutsceneHideTrackedForeground(M::CutsceneFlat,true,true,false,false,false,true,true));
    CHECK(!CutsceneHideTrackedForeground(M::CutsceneFlat,true,true,false,true,false,false,true));
    CHECK(!CutsceneHideTrackedForeground(M::CutsceneFlat,true,true,false,true,false,true,false));
    CHECK(!CutsceneHideTrackedForeground(M::CutsceneFlat,false,true,false,true,false,true,true));
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
    // PID5052's confirmed movie/loading -> first-world camera changes pawn/menu
    // identities. They are expected while native cinematic ON remains present.
    CHECK((!CameraChangeClearsCine(C{ true, false, false, true, true })));
    CHECK((!CameraChangeClearsCine(C{ true, false, true, false, true })));
    CHECK((CameraChangeClearsCine(C{ true, false, false, true, false })));
    CHECK((CameraChangeClearsCine(C{ true, true, true, true, false })));
}

int main() {
    test_sbs_fit();
    test_odd_dims();
    test_degenerate();
    test_smaller_cinema_and_projection();
    test_predicate();
    test_camera_change_gate();
    if (g_failures == 0) { std::printf("PASS: cutscene mono fit + predicate\n"); return 0; }
    std::printf("FAILED: %d checks\n", g_failures);
    return 1;
}
