// Tests for glove isolation only (no D3D, no game).
// Covers: bone classification (Keep/Collapse/Review), collapse math (S*D
// order, every point maps onto the wrist center), side mapping, and the
// fail-safe rule (unknown bones are never collapsed).
#include <cstdio>
#include <cstdlib>
#include <cmath>

#include "../../../spikes/view_matrix/glove_isolate.h"

static int g_fail = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", msg); ++g_fail; }
}
static bool Near(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) <= eps; }

using gloveisolate::BoneRole;

int main() {
    using namespace gloveisolate;
    // Keep: gloves (wrist subtree) + TMD device + utilities.
    Check(Classify("l_arm_wrist") == BoneRole::Keep, "wrist kept");
    Check(Classify("r_index_2") == BoneRole::Keep, "finger kept");
    Check(Classify("l_pinky_cup") == BoneRole::Keep, "pinky cup kept");
    Check(Classify("tmd_base") == BoneRole::Keep, "TMD base kept");
    Check(Classify("hinge1") == BoneRole::Keep, "TMD hinge kept");
    Check(Classify("rail_rot") == BoneRole::Keep, "TMD rail kept");
    Check(Classify("nixie1") == BoneRole::Keep, "TMD nixie kept");
    Check(Classify("faceplate") == BoneRole::Keep, "TMD faceplate kept");
    Check(Classify("cog_gun") == BoneRole::Keep, "gun bone kept");
    // Collapse: arm flesh + sleeve.
    Check(Classify("l_arm_forearm") == BoneRole::Collapse, "forearm collapsed");
    Check(Classify("r_arm_elbow") == BoneRole::Collapse, "elbow collapsed");
    Check(Classify("l_arm_shoulder") == BoneRole::Collapse, "shoulder collapsed");
    Check(Classify("l_arm_forearm2") == BoneRole::Collapse, "forearm2 collapsed");
    Check(Classify("sleeve_fix_3") == BoneRole::Collapse, "sleeve fix collapsed");
    // Fail-safe: unknown or empty names are reviewed, never collapsed.
    Check(Classify("mystery_bone") == BoneRole::Review, "unknown reviewed");
    Check(Classify("") == BoneRole::Review, "empty reviewed");
    Check(Classify(nullptr) == BoneRole::Review, "null reviewed");
    // Sides.
    Check(HandSide("l_index_1") == 0, "left side");
    Check(HandSide("r_thumb_2") == 1, "right side");
    Check(HandSide("sleeve_fix_1") == 0, "sleeve fix maps left");
    Check(HandSide("tmd_base") == -1, "device has no side");
    // Collapse math: every point maps exactly onto the wrist center.
    Mat4 d;
    const float w[3] = {1.0f, 2.0f, 3.0f};
    CollapseDelta(w, d);
    Check(Near(d.m[3][3], 1.0f), "homogeneous row preserved");
    const float pts[][3] = {{0, 0, 0}, {5, -2, 9}, {1, 2, 3}, {-100, 0.5f, 42}};
    for (auto& p : pts) {
        float o[3];
        TransformPoint(d.m, p, o);
        Check(Near(o[0], w[0]) && Near(o[1], w[1]) && Near(o[2], w[2]),
              "collapse maps to wrist");
    }
    // S*D order: identity skin becomes the collapse itself.
    float ident[4][4] = {};
    for (int i = 0; i < 4; ++i) ident[i][i] = 1.0f;
    float out[4][4];
    ApplyDelta(ident, d, out);
    Check(Near(out[3][0], 1.0f) && Near(out[3][3], 1.0f), "identity*D keeps wrist + w=1");
    Check(Near(out[0][0], 0.0f) && Near(out[1][2], 0.0f), "identity*D zeroes rotation rows");

    if (g_fail) { std::fprintf(stderr, "glove isolation tests: %d FAILED\n", g_fail); return 1; }
    std::puts("PASS: glove isolation (keep/collapse/review + collapse math)");
    return 0;
}
