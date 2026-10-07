// Deterministic tests for NIS upscaling bridge math (research/nvidia-vr-upscaling).
//
// Covers: INI mode parsing + fallback, sharpness/render/output clamping,
// resolution calculations (1.0/0.85/0.77/0.70), alignment, left/right source
// rectangles (SBS seam isolation), output sizing (never from render size),
// resource-recreation predicate, and the official NVScalerUpdateConfig host
// math (scale factors, viewport handling, out-of-range rejection).
//
// No D3D, no game, no headset. Build: cl /EHsc /O2 test-nis-upscale.cpp
// Run: test-nis-upscale.exe (exit 0 = all pass).

#include <cstdio>
#include <cstring>
#include <cmath>

#include "../nis_upscale.h"
#include "../nis/NIS_Config.h"

static int g_fail = 0;
static int g_pass = 0;

#define CHECK(cond) do { \
    if (cond) { ++g_pass; } \
    else { ++g_fail; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } \
} while (0)

#define CHECK_NEAR(a, b, eps) CHECK(std::fabs((double)(a) - (double)(b)) <= (eps))

static void TestParseMode() {
    using nis_upscale::ParseMode;
    using nis_upscale::kOff;
    using nis_upscale::kNIS;
    using nis_upscale::kNISSharpen;
    CHECK(ParseMode("Off") == kOff);
    CHECK(ParseMode("off") == kOff);
    CHECK(ParseMode("OFF") == kOff);
    CHECK(ParseMode("NIS") == kNIS);
    CHECK(ParseMode("nis") == kNIS);
    CHECK(ParseMode("Nis") == kNIS);
    CHECK(ParseMode("NISSharpen") == kNISSharpen);
    CHECK(ParseMode("nissharpen") == kNISSharpen);
    CHECK(ParseMode("NIS_Sharpen") == kNISSharpen);
    CHECK(ParseMode("nis-sharpen") == kNISSharpen);
    CHECK(ParseMode("sharpen") == kNISSharpen);
    CHECK(ParseMode("") == kOff);
    CHECK(ParseMode(nullptr) == kOff);
    CHECK(ParseMode("DLSS") == kOff);      // unknown -> safe fallback
    CHECK(ParseMode("nisss") == kOff);
    CHECK(ParseMode("2") == kOff);
    CHECK(0 == strcmp(nis_upscale::ModeName(kOff), "Off"));
    CHECK(0 == strcmp(nis_upscale::ModeName(kNIS), "NIS"));
    CHECK(0 == strcmp(nis_upscale::ModeName(kNISSharpen), "NISSharpen"));
}

static void TestClamps() {
    CHECK(nis_upscale::ClampSharpness(0.35f) == 0.35f);
    CHECK(nis_upscale::ClampSharpness(-1.0f) == 0.0f);
    CHECK(nis_upscale::ClampSharpness(2.0f) == 1.0f);
    CHECK(nis_upscale::ClampSharpness(0.0f) == 0.0f);
    CHECK(nis_upscale::ClampSharpness(1.0f) == 1.0f);
    CHECK(nis_upscale::ClampRenderScale(1.0f) == 1.0f);
    CHECK(nis_upscale::ClampRenderScale(0.2f) == 0.5f);   // floor
    CHECK(nis_upscale::ClampRenderScale(1.5f) == 1.0f);   // ceiling
    CHECK(nis_upscale::ClampOutputScale(1.0f) == 1.0f);
    CHECK(nis_upscale::ClampOutputScale(0.5f) == 1.0f);   // never shrink swapchain
    CHECK(nis_upscale::ClampOutputScale(2.0f) == 1.5f);   // ceiling
    CHECK(nis_upscale::ClampOutputScale(1.2f) == 1.2f);
}

static void TestRenderSizes() {
    // recSBS 5264x2684 (2632x2684 per eye, PSVR2-class).
    uint32_t w = 0, h = 0;
    nis_upscale::ComputeRenderSBS(5264, 2684, 1.00f, &w, &h);
    CHECK(w == 5264 && h == 2684);
    nis_upscale::ComputeRenderSBS(5264, 2684, 0.85f, &w, &h);
    CHECK(w == 4476 && h == 2282);
    nis_upscale::ComputeRenderSBS(5264, 2684, 0.77f, &w, &h);
    CHECK(w == 4056 && h == 2068);
    nis_upscale::ComputeRenderSBS(5264, 2684, 0.70f, &w, &h);
    CHECK(w == 3688 && h == 1880);
    // Alignment + bounds: SBS width multiple of 4, height even.
    for (float s = 0.5f; s <= 1.001f; s += 0.07f) {
        nis_upscale::ComputeRenderSBS(5264, 2684, s, &w, &h);
        CHECK(w % 4 == 0);
        CHECK(h % 2 == 0);
    }
    // Aspect preserved (same normalized scale): within alignment slop.
    {
        uint32_t w1, h1;
        nis_upscale::ComputeRenderSBS(5264, 2684, 0.77f, &w1, &h1);
        double a0 = 5264.0 / 2684.0, a1 = (double)w1 / (double)h1;
        CHECK_NEAR(a0, a1, 0.01);
    }
    // Degenerate input -> zeros, never garbage.
    nis_upscale::ComputeRenderSBS(0, 0, 0.77f, &w, &h);
    CHECK(w == 0 && h == 0);
}

static void TestEyeRects() {
    // Full SBS must be exactly covered by two disjoint halves.
    const uint32_t cases[][2] = { {5264,2684}, {4056,2068}, {4476,2282}, {3688,1880} };
    for (auto& c : cases) {
        uint32_t sbsW = c[0], sbsH = c[1];
        uint32_t ew = 0, eh = 0;
        nis_upscale::EyeInputFromSBS(sbsW, sbsH, &ew, &eh);
        CHECK(ew == sbsW / 2 && eh == sbsH);
        auto L = nis_upscale::EyeSrcRect(0, sbsW, sbsH);
        auto R = nis_upscale::EyeSrcRect(1, sbsW, sbsH);
        CHECK(L.x == 0 && L.y == 0 && L.w == sbsW / 2 && L.h == sbsH);
        CHECK(R.x == sbsW / 2 && R.y == 0 && R.w == sbsW / 2 && R.h == sbsH);
        // Seam isolation: left end == right start, no overlap, no gap.
        CHECK(L.x + L.w == R.x);
        CHECK(L.x + L.w + R.w == sbsW);
        // No shared pixels: intervals [L.x, L.x+L.w) and [R.x, R.x+R.w) disjoint.
        CHECK(!(L.x < R.x + R.w && R.x < L.x + L.w) || (L.x + L.w == R.x));
    }
}

static void TestOutputEye() {
    uint32_t w = 0, h = 0;
    // From recommended, never from render size.
    nis_upscale::ComputeOutputEye(2632, 2684, 1.0f, 2028, 2068, &w, &h);
    CHECK(w == 2632 && h == 2684);
    nis_upscale::ComputeOutputEye(2632, 2684, 1.2f, 2028, 2068, &w, &h);
    CHECK(w == 3158 && h == 3222);  // 2632*1.2=3158.4->3158; 2684*1.2=3220.8->3221->3222
    // No recommended (fallback): pass through the render eye size.
    nis_upscale::ComputeOutputEye(0, 0, 1.0f, 2028, 2068, &w, &h);
    CHECK(w == 2028 && h == 2068);
}

static void TestNeedsRecreate() {    using nis_upscale::NeedsRecreate;
    CHECK(!NeedsRecreate(2028, 2068, 2632, 2684, 2028, 2068, 2632, 2684));
    CHECK(NeedsRecreate(2028, 2068, 2632, 2684, 2238, 2282, 2632, 2684));  // input changed
    CHECK(NeedsRecreate(2028, 2068, 2632, 2684, 2028, 2068, 3160, 3222));  // output changed
    CHECK(NeedsRecreate(0, 0, 0, 0, 2028, 2068, 2632, 2684));              // first create
}

static void TestOfficialConfig() {
    // Official host math: 2028x2068 per eye -> 2632x2684.
    NISConfig cfg = {};
    bool ok = NVScalerUpdateConfig(cfg, 0.35f,
        /*inOx*/0, /*inOy*/0, /*inW*/2028, /*inH*/2068, /*texW*/4056, /*texH*/2068,
        /*outOx*/0, /*outOy*/0, /*outW*/2632, /*outH*/2684, /*oTexW*/2632, /*oTexH*/2684,
        NISHDRMode::None);
    CHECK(ok);
    CHECK_NEAR(cfg.kScaleX, 2028.0 / 2632.0, 1e-6);
    CHECK_NEAR(cfg.kScaleY, 2068.0 / 2684.0, 1e-6);
    // Viewport origin for the RIGHT eye inside the SBS texture.
    NISConfig right = {};
    ok = NVScalerUpdateConfig(right, 0.35f,
        2028, 0, 2028, 2068, 4056, 2068,
        0, 0, 2632, 2684, 2632, 2684,
        NISHDRMode::None);
    CHECK(ok);
    CHECK(right.kInputViewportOriginX == 2028);
    CHECK(right.kInputViewportOriginY == 0);
    // Out-of-range scale rejected (upscale-only SDK: kScale must be 0.5..1).
    NISConfig bad = {};
    CHECK(!NVScalerUpdateConfig(bad, 0.35f,
        0, 0, 2632, 2684, 5264, 2684,
        0, 0, 6000, 2684, 6000, 2684,
        NISHDRMode::None));
    // Sharpen-only path accepts same-size viewports.
    NISConfig sh = {};
    CHECK(NVSharpenUpdateConfig(sh, 0.35f, 0, 0, 2632, 2684, 2632, 2684, 0, 0));
}

static void TestShouldEngage() {
    using nis_upscale::ShouldEngageUpscale;
    using nis_upscale::kOff;
    using nis_upscale::kNIS;
    using nis_upscale::kNISSharpen;
    // Off-mode regression: Off NEVER engages, whatever the rest claims.
    CHECK(!ShouldEngageUpscale(kOff, true, false, true, true));
    CHECK(!ShouldEngageUpscale(kOff, true, false, false, false));
    CHECK(!ShouldEngageUpscale(kOff, false, true, true, true));
    // Fully armed NIS / NISSharpen engage.
    CHECK(ShouldEngageUpscale(kNIS, true, false, true, true));
    CHECK(ShouldEngageUpscale(kNISSharpen, true, false, true, true));
    // Each single missing precondition disengages.
    CHECK(!ShouldEngageUpscale(kNIS, false, false, true, true));   // not ready
    CHECK(!ShouldEngageUpscale(kNIS, true, true, true, true));     // failedOnce
    CHECK(!ShouldEngageUpscale(kNIS, true, false, false, true));   // mono/menu
    CHECK(!ShouldEngageUpscale(kNIS, true, false, true, false));   // no source
    CHECK(!ShouldEngageUpscale(99, true, false, true, true));      // bogus mode
}

static void TestSeamBounds() {
    // Every eye viewport must lie inside the source texture: origin + size
    // <= dimension on both axes. A high-contrast SBS seam would reveal any
    // cross-eye tap as color bleed; these bounds are what prevents it,
    // together with the clamp-to-edge sampler.
    const uint32_t widths[] = { 5264, 4476, 4056, 3688, 4054, 4055, 640 };
    for (uint32_t sbsW : widths) {
        for (int eye = 0; eye < 2; ++eye) {
            auto r = nis_upscale::EyeSrcRect(eye, sbsW, 2684);
            CHECK(r.x + r.w <= sbsW);
            CHECK(r.y + r.h <= 2684);
            CHECK(r.w > 0 && r.h > 0);
        }
        auto L = nis_upscale::EyeSrcRect(0, sbsW, 2684);
        auto R = nis_upscale::EyeSrcRect(1, sbsW, 2684);
        CHECK(L.x + L.w == R.x);   // adjacent, never overlapping
        // Odd widths leave one remainder column unsampled (never duplicated).
        CHECK(R.x + R.w <= sbsW);
        CHECK(sbsW - (R.x + R.w) <= 1);
    }
    // Official config honors the same bounds for the right eye.
    NISConfig right = {};
    CHECK(NVScalerUpdateConfig(right, 0.35f,
        2028, 0, 2028, 2068, 4056, 2068,
        0, 0, 2632, 2684, 2632, 2684,
        NISHDRMode::None));
    CHECK(right.kInputViewportOriginX + right.kInputViewportWidth <= 4056);
}

int main() {
    TestParseMode();
    TestClamps();
    TestRenderSizes();
    TestEyeRects();
    TestOutputEye();
    TestNeedsRecreate();
    TestShouldEngage();
    TestSeamBounds();
    TestOfficialConfig();
    std::printf("nis-upscale tests: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
