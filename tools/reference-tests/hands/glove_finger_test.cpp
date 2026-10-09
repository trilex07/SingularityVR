// Tests for glove isolation + procedural finger posing (no D3D, no game).
// Covers: bone classification, collapse math (S*D order), flexion rotation
// invariants, the baked axis table, curl mapping/smoothing, input fallbacks,
// and slot/descendant resolution on synthetic skeletons.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include "../../../spikes/view_matrix/glove_isolate.h"
#include "../../../spikes/view_matrix/finger_pose.h"

static int g_fail = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", msg); ++g_fail; }
}
static bool Near(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) <= eps; }

using gloveisolate::BoneRole;

static void TestClassify() {
    using namespace gloveisolate;
    Check(Classify("l_arm_wrist") == BoneRole::Keep, "wrist kept");
    Check(Classify("r_index_2") == BoneRole::Keep, "finger kept");
    Check(Classify("l_pinky_cup") == BoneRole::Keep, "pinky cup kept");
    Check(Classify("l_arm_forearm") == BoneRole::Collapse, "forearm collapsed");
    Check(Classify("r_arm_elbow") == BoneRole::Collapse, "elbow collapsed");
    Check(Classify("l_arm_shoulder") == BoneRole::Collapse, "shoulder collapsed");
    Check(Classify("sleeve_fix_3") == BoneRole::Collapse, "sleeve fix collapsed");
    Check(Classify("tmd_base") == BoneRole::Keep, "TMD base kept");
    Check(Classify("hinge1") == BoneRole::Keep, "TMD hinge kept");
    Check(Classify("rail_rot") == BoneRole::Keep, "TMD rail kept");
    Check(Classify("nixie1") == BoneRole::Keep, "TMD nixie kept");
    Check(Classify("faceplate") == BoneRole::Keep, "TMD faceplate kept");
    Check(Classify("cog_gun") == BoneRole::Keep, "gun bone kept");
    Check(Classify("mystery_bone") == BoneRole::Review, "unknown reviewed, never collapsed");
    Check(Classify("") == BoneRole::Review, "empty reviewed");
    Check(HandSide("l_index_1") == 0, "left side");
    Check(HandSide("r_thumb_2") == 1, "right side");
    Check(HandSide("sleeve_fix_1") == 0, "sleeve fix maps left");
}

static void TestCollapse() {
    using namespace gloveisolate;
    Mat4 d;
    const float w[3] = {1.0f, 2.0f, 3.0f};
    CollapseDelta(w, d);
    // Any point maps exactly onto w (row-vector convention).
    const float pts[][3] = {{0, 0, 0}, {5, -2, 9}, {1, 2, 3}, {-100, 0.5f, 42}};
    for (auto& p : pts) {
        float o[3];
        TransformPoint(d.m, p, o);
        Check(Near(o[0], w[0]) && Near(o[1], w[1]) && Near(o[2], w[2]), "collapse maps to wrist");
    }
    // S*D order: identity skin becomes the collapse itself.
    float ident[4][4] = {};
    for (int i = 0; i < 4; ++i) ident[i][i] = 1.0f;
    float out[4][4];
    ApplyDelta(ident, d, out);
    Check(Near(out[3][0], 1.0f) && Near(out[3][3], 1.0f), "identity*D keeps wrist + w=1");
    Check(Near(out[0][0], 0.0f) && Near(out[1][2], 0.0f), "identity*D zeroes rotation rows");
}

static void TestFlexRotation() {
    using namespace fingerpose;
    using gloveisolate::Mat4;
    // (1,0,0) about +Z by +90deg -> (0,1,0): same formula as offline verification.
    Mat4 d;
    const float c[3] = {0, 0, 0}, z[3] = {0, 0, 1};
    FlexDelta(c, z, 3.14159265f * 0.5f, d);
    float o[3];
    const float p[3] = {1, 0, 0};
    gloveisolate::TransformPoint(d.m, p, o);
    Check(Near(o[0], 0.0f, 1e-3f) && Near(o[1], 1.0f, 1e-3f) && Near(o[2], 0.0f, 1e-3f),
          "row-vector Rodrigues matches the verified convention");
    // Rotation about c fixes c and preserves distances.
    const float c2[3] = {4, -2, 7}, ax[3] = {0.3f, -0.8f, 0.5f};
    FlexDelta(c2, ax, 1.1f, d);
    gloveisolate::TransformPoint(d.m, c2, o);
    Check(Near(o[0], c2[0]) && Near(o[1], c2[1]) && Near(o[2], c2[2]), "center fixed");
    const float q[3] = {9, 1, -3};
    float q2[3];
    gloveisolate::TransformPoint(d.m, q, q2);
    float d0 = std::sqrt((q[0]-c2[0])*(q[0]-c2[0]) + (q[1]-c2[1])*(q[1]-c2[1]) + (q[2]-c2[2])*(q[2]-c2[2]));
    float d1 = std::sqrt((q2[0]-c2[0])*(q2[0]-c2[0]) + (q2[1]-c2[1])*(q2[1]-c2[1]) + (q2[2]-c2[2])*(q2[2]-c2[2]));
    Check(Near(d0, d1, 1e-3f), "rigid: distances preserved");
    // Zero angle = identity; degenerate axis falls back safely.
    FlexDelta(c2, ax, 0.0f, d);
    gloveisolate::TransformPoint(d.m, q, q2);
    Check(Near(q2[0], q[0]) && Near(q2[1], q[1]) && Near(q2[2], q[2]), "zero angle is identity");
    const float bad[3] = {0, 0, 0};
    FlexDelta(c2, bad, 0.5f, d);  // must not produce NaN
    gloveisolate::TransformPoint(d.m, q, q2);
    Check(q2[0] == q2[0] && q2[1] == q2[1] && q2[2] == q2[2], "degenerate axis is finite");
}

static void TestTable() {
    using namespace fingerpose;
    Check(kFingerCount == 10, "ten fingers baked");
    int left = 0, right = 0;
    for (int i = 0; i < kFingerCount; ++i) {
        const FingerDef& f = kFingers[i];
        if (f.hand == 0) ++left; else ++right;
        Check(f.nJoints >= 3 && f.joints, "finger has joints");
        for (int j = 0; j < f.nJoints; ++j) {
            const FingerJoint& jt = f.joints[j];
            const float l = std::sqrt(jt.axis[0]*jt.axis[0] + jt.axis[1]*jt.axis[1] + jt.axis[2]*jt.axis[2]);
            Check(Near(l, 1.0f, 1e-3f), "baked axis is unit");
            Check(jt.maxDeg > 0.0f && jt.maxDeg <= 110.0f, "max angle sane");
            Check(jt.bone && jt.bone[0], "joint has a bone name");
        }
    }
    Check(left == 5 && right == 5, "five fingers per hand");
}

static void TestCurls() {
    using namespace fingerpose;
    Check(Near(CurlOf(CurlState{0.1f, 0.2f, 0.3f, 0.4f, 0.5f}, "l_index_2"), 0.1f), "index maps");
    Check(Near(CurlOf(CurlState{0.1f, 0.2f, 0.3f, 0.4f, 0.5f}, "r_thumb_1"), 0.5f), "thumb maps");
    CurlState s;
    CurlInput t;
    t.index = 1.0f; t.middle = t.ring = t.pinky = t.thumb = 0.0f;
    for (int i = 0; i < 200; ++i) SmoothCurls(s, t, 1.0f / 60.0f);
    Check(s.index > 0.9f && s.middle < 0.05f, "smoothing converges without overshoot");
    CurlInput bad;
    bad.index = 5.0f; bad.thumb = -2.0f;
    SmoothCurls(s, bad, 1.0f / 60.0f);
    Check(s.index <= 1.0f && s.thumb >= 0.0f, "inputs clamped");
    // Input builder fallbacks.
    CurlInput o;
    BuildInput(0.7f, 0.4f, true, true, true, false, true, o);
    Check(Near(o.index, 0.7f) && Near(o.middle, 0.4f) && o.thumb < 0.45f, "touch parks the thumb");
    BuildInput(0.7f, 0.4f, true, true, false, false, false, o);
    Check(Near(o.thumb, 0.45f + 0.30f * 0.4f, 1e-4f), "no-touch degrades to analog pose");
    BuildInput(0.7f, 0.4f, false, false, false, false, true, o);
    Check(Near(o.index, 0.0f) && Near(o.middle, 0.0f), "missing analogs read open");
}

static void TestResolve() {
    using namespace fingerpose;
    RefBone refs[] = {
        {"Root", 0}, {"l_arm_wrist", 0}, {"l_index_1", 1}, {"l_index_2", 2}, {"l_index_3", 3},
    };
    const unsigned map[] = {1, 2, 3, 4};
    Check(FindSlot(map, 4, refs, 5, "l_index_2") == 2, "slot found by bone name");
    Check(FindSlot(map, 4, refs, 5, "nope") == -1, "missing name returns -1");
    Check(IsDescendantOrSelf(4, 2, refs, 5), "tip descends from PIP");
    Check(IsDescendantOrSelf(2, 2, refs, 5), "self counts");
    Check(!IsDescendantOrSelf(1, 3, refs, 5), "MCP is not under DIP");
}

int main() {
    TestClassify();
    TestCollapse();
    TestFlexRotation();
    TestTable();
    TestCurls();
    TestResolve();
    if (g_fail) { std::fprintf(stderr, "glove/finger tests: %d FAILED\n", g_fail); return 1; }
    std::puts("PASS: glove isolation + finger posing");
    return 0;
}
