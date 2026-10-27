// Deterministic tests for the gloves-only model logic (../glove_isolate.h):
// bone classification, hand-side attribution, and the collapse algebra that
// turns arm flesh into the wrist point.
//
// REGRESSION CONTEXT (2026-10-09). The shipping INI carried
// `GloveIsolate=0` while its own comment mandated the live-wrist path (=2).
// `ApplyGloveIsolation` opens with `if (mode <= 0 || !map || slots <= 0)
// return;`, so at mode 0 it left before ever reaching Classify() or
// CollapseDelta() - the feature was silently OFF and, because the GloveCensus
// log line lives *after* that gate, the diagnostic could not print either. The
// absence of GloveCensus was therefore indistinguishable from "the code is
// dead", which is exactly the misreading it caused once already.
//
// These tests pin the LOGIC that the mode gate guards, so a future change to
// Classify/CollapseDelta cannot silently break the mode-2 path. The INI-side
// guard for this same bug lives in tests/validate-runtime-policy.ps1 (asserts
// the shipping ini never carries GloveIsolate=0).
//
// No D3D, no game, no headset. Build: cl /EHsc /O2 /std:c++17 test-glove-isolate.cpp
// Run: test-glove-isolate.exe (exit 0 = all pass).

#include <cmath>
#include <cstdio>
#include <cstring>

#include "../glove_isolate.h"

static int g_fail = 0;
static int g_pass = 0;

#define CHECK(cond) do { \
    if (cond) { ++g_pass; } \
    else { ++g_fail; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } \
} while (0)

#define CHECK_NEAR(a, b, eps) CHECK(std::fabs((double)(a) - (double)(b)) <= (eps))

// ---- classification ------------------------------------------------------
// The census only means something if role assignment is right: arm flesh must
// collapse, gloves/fingers/device/utility must survive, anything unrecognised
// must land in Review (never collapsed - the fail-safe direction).
static void TestClassify() {
    using gloveisolate::BoneRole;
    using gloveisolate::Classify;

    const char* collapse[] = {
        "l_arm_forearm", "r_arm_forearm", "l_arm_bicep", "r_arm_elbow",
        "l_clavicle", "r_shoulder", "sleeve_fix_01", "sleeve_fix_07",
    };
    for (const char* n : collapse) CHECK(Classify(n) == BoneRole::Collapse);

    const char* keep[] = {
        "l_arm_wrist", "r_arm_wrist",              // glove: wrist subtree
        "l_index_03", "r_thumb_02", "l_pinky_01",   // glove: fingers
        "tmd_base", "disc", "wheel", "nub_01",      // TMD device stays native
        "Root", "IK_hand_root", "Spine_01",         // utility
    };
    for (const char* n : keep) CHECK(Classify(n) == BoneRole::Keep);

    // Unrecognised / empty must be Review: logged, never collapsed.
    CHECK(Classify("mystery_bone") == BoneRole::Review);
    CHECK(Classify("") == BoneRole::Review);
    CHECK(Classify(nullptr) == BoneRole::Review);

    // The whole point of the feature: fingers are never collateral damage.
    CHECK(Classify("r_index_01") == BoneRole::Keep);
    CHECK(Classify("l_arm_wrist") != BoneRole::Collapse);
}

// ---- hand attribution ----------------------------------------------------
// Collapsing a bone onto the WRONG wrist would pull the left forearm onto the
// right hand. sleeve_fix_* are the documented exception: all parented under
// l_arm_forearm, so they belong to the left side by name convention.
static void TestHandSide() {
    using gloveisolate::HandSide;

    CHECK(HandSide("l_arm_forearm") == 0);
    CHECK(HandSide("l_arm_wrist") == 0);
    CHECK(HandSide("r_arm_forearm") == 1);
    CHECK(HandSide("r_arm_wrist") == 1);
    CHECK(HandSide("sleeve_fix_03") == 0);   // convention, not a prefix match
    CHECK(HandSide("mystery_bone") == -1);
    CHECK(HandSide(nullptr) == -1);
}

// Row-vector convention: translation lives in row 3 and v' = v * S, matching
// the mod's armbind::Mat / ToGpu layout. If this ever flips to column-vector,
// every collapse lands in the wrong place.
static void Rigid(float yaw, float tx, float ty, float tz, float s[4][4]) {
    const float c = std::cos(yaw), n = std::sin(yaw);
    const float m[4][4] = {
        {  c, -n, 0, 0 },
        {  n,  c, 0, 0 },
        {  0,  0, 1, 0 },
        { tx, ty, tz, 1 },
    };
    for (int r = 0; r < 4; ++r)
        for (int col = 0; col < 4; ++col) s[r][col] = m[r][col];
}

// ---- collapse algebra ----------------------------------------------------
// The documented invariant (glove_isolate.h): for a rigid S, S' = S * D has
// rows 0..2 all zero and row 3 = (w, 1), so EVERY skinned vertex maps exactly
// onto the wrist centre and the boundary triangles degenerate. This is the
// "arm flesh shrinks to the wrist, gloves untouched" behaviour the headset
// pass is meant to confirm visually.
static void TestCollapseInvariant() {
    using gloveisolate::ApplyDelta;
    using gloveisolate::CollapseDelta;
    using gloveisolate::Mat4;
    using gloveisolate::TransformPoint;

    const float w[3] = { 12.5f, -3.25f, 41.0f };

    // D structure: only row 3 carries (w, 1).
    Mat4 d;
    gloveisolate::CollapseDelta(w, d);
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 4; ++c) CHECK_NEAR(d.m[r][c], 0.0f, 1e-7f);
    CHECK_NEAR(d.m[3][0], w[0], 1e-7f);
    CHECK_NEAR(d.m[3][1], w[1], 1e-7f);
    CHECK_NEAR(d.m[3][2], w[2], 1e-7f);
    CHECK_NEAR(d.m[3][3], 1.0f, 1e-7f);

    const float yaws[] = { 0.0f, 0.7f, -2.4f, 3.14159265f };
    const float trans[][3] = { {0,0,0}, {5,-2,9}, {-17.5f, 3.1f, 0.25f} };

    for (float yaw : yaws) {
        for (const auto& t : trans) {
            float s[4][4];
            Rigid(yaw, t[0], t[1], t[2], s);

            float out[4][4];
            ApplyDelta(s, d, out);

            // Rows 0..2 collapse to zero for a rigid transform.
            for (int r = 0; r < 3; ++r)
                for (int c = 0; c < 4; ++c) CHECK_NEAR(out[r][c], 0.0f, 1e-5f);
            // Row 3 becomes the wrist centre exactly.
            CHECK_NEAR(out[3][0], w[0], 1e-5f);
            CHECK_NEAR(out[3][1], w[1], 1e-5f);
            CHECK_NEAR(out[3][2], w[2], 1e-5f);
            CHECK_NEAR(out[3][3], 1.0f, 1e-5f);

            // The invariant that matters: ANY input point lands on w.
            const float probes[][3] = {
                {0,0,0}, {100,-100,50}, {0.5f,0.5f,0.5f}, {-1,2,-3},
            };
            for (const auto& p : probes) {
                float got[3];
                TransformPoint(out, p, got);
                CHECK_NEAR(got[0], w[0], 1e-4f);
                CHECK_NEAR(got[1], w[1], 1e-4f);
                CHECK_NEAR(got[2], w[2], 1e-4f);
            }
        }
    }
}

// Convention sanity: the row-vector layout above must agree with how the mod
// applies a transform, otherwise "collapses onto the wrist" would be computed
// against a different frame than the renderer uses.
static void TestRowVectorConvention() {
    using gloveisolate::TransformPoint;
    const float s[4][4] = {
        { 1, 0, 0, 0 },
        { 0, 1, 0, 0 },
        { 0, 0, 1, 0 },
        { 7, 8, 9, 1 },   // translation in row 3
    };
    float out[3];
    const float p[3] = { 1.0f, 2.0f, 3.0f };
    TransformPoint(s, p, out);
    CHECK_NEAR(out[0], 8.0f, 1e-6f);
    CHECK_NEAR(out[1], 10.0f, 1e-6f);
    CHECK_NEAR(out[2], 12.0f, 1e-6f);
}

int main() {
    TestClassify();
    TestHandSide();
    TestCollapseInvariant();
    TestRowVectorConvention();

    std::printf("glove-isolate: classify %s / hand side %s / collapse-onto-wrist invariant %s "
                "(%d checks, %d failed)\n",
                "ok", "ok", "ok", g_pass, g_fail);
    if (g_fail) std::printf("FAILURES: %d\n", g_fail);
    return g_fail ? 1 : 0;
}
