// Tests for glove isolation only (no D3D, no game).
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
    Check(IsStrayFingerSection(390, 14490, 674), "stray sleeve/finger section identified");
    Check(IsStrayFingerSection(3314, 14490, 674), "alternate stray section identity identified");
    Check(!IsStrayFingerSection(3314, 0, 4830), "main glove section preserved");
    Check(!IsStrayFingerSection(7444, 13206, 7704), "TMD section preserved");
    Check(Classify("l_arm_wrist") == BoneRole::Keep, "wrist kept");
    Check(Classify("r_index_2") == BoneRole::Keep, "finger kept");
    Check(Classify("l_pinky_cup") == BoneRole::Keep, "pinky cup kept");
    Check(Classify("tmd_base") == BoneRole::Keep, "TMD base kept");
    Check(Classify("l_arm_forearm") == BoneRole::Collapse, "forearm collapsed");
    Check(Classify("r_arm_elbow") == BoneRole::Collapse, "elbow collapsed");
    Check(Classify("sleeve_fix_3") == BoneRole::Collapse, "sleeve fix collapsed");
    Check(Classify("mystery_bone") == BoneRole::Review, "unknown reviewed");
    Check(Classify(nullptr) == BoneRole::Review, "null reviewed");
    Check(HandSide("l_index_1") == 0, "left side");
    Check(HandSide("r_thumb_2") == 1, "right side");
    Check(HandSide("sleeve_fix_1") == 0, "sleeve fix maps left");
    Check(ControllerDeltaForBone("l_index_1", 0) == 0, "right-handed left bone uses off-hand delta");
    Check(ControllerDeltaForBone("r_index_1", 0) == 1, "right-handed right bone uses aim-hand delta");
    Check(ControllerDeltaForBone("l_index_1", 1) == 1, "left-handed left bone uses aim-hand delta");
    Check(ControllerDeltaForBone("r_index_1", 1) == 0, "left-handed right bone uses off-hand delta");
    Check(ControllerDeltaForBone("mystery_bone", 0) == -1, "unknown bone has no controller delta");

    Mat4 d0;
    const float w[3] = {1.0f, 2.0f, 3.0f};
    CollapseDelta(w, d0, 0.0f);
    Check(Near(d0.m[3][3], 1.0f), "homogeneous row preserved");
    const float pts[][3] = {{0, 0, 0}, {5, -2, 9}, {1, 2, 3}, {-100, 0.5f, 42}};
    for (auto& p : pts) {
        float o[3];
        TransformPoint(d0.m, p, o);
        Check(Near(o[0], w[0]) && Near(o[1], w[1]) && Near(o[2], w[2]),
              "point collapse maps to wrist");
    }
    float ident[4][4] = {};
    for (int i = 0; i < 4; ++i) ident[i][i] = 1.0f;
    float out[4][4];
    ApplyDelta(ident, d0, out);
    Check(Near(out[3][0], 1.0f) && Near(out[3][3], 1.0f), "identity*D keeps wrist + w=1");
    Check(Near(out[0][0], 0.0f) && Near(out[1][2], 0.0f), "identity*D zeroes rotation rows");

    Mat4 d;
    CollapseDelta(w, d);
    float ow[3];
    TransformPoint(d.m, w, ow);
    Check(Near(ow[0], w[0]) && Near(ow[1], w[1]) && Near(ow[2], w[2]), "stump fixes wrist");
    const float p[3] = {11.0f, 2.0f, 3.0f};
    float o[3];
    TransformPoint(d.m, p, o);
    Check(Near(o[0], 0.12f * 11.0f + 0.88f * 1.0f, 1e-3f) && Near(o[1], 2.0f) && Near(o[2], 3.0f),
          "stump scales toward wrist");
    Check(d.m[0][0] > 0.0f && d.m[3][3] == 1.0f, "stump keeps volume + homogeneous row");
    Mat4 di;
    CollapseDelta(w, di, 1.0f);
    TransformPoint(di.m, p, o);
    Check(Near(o[0], p[0]) && Near(o[1], p[1]) && Near(o[2], p[2]), "s=1 is identity");

    if (g_fail) { std::fprintf(stderr, "glove isolation tests: %d FAILED\n", g_fail); return 1; }
    std::puts("PASS: glove isolation (keep/collapse/review + collapse math)");
    return 0;
}
