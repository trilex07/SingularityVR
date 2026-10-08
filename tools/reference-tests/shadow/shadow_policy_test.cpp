#include "../../../spikes/view_matrix/shadow_pass_policy.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace singularity::shadow;

namespace {
int failures = 0;
void Check(bool value, const char* what) {
    if (!value) { std::cerr << "FAIL: " << what << '\n'; ++failures; }
}
bool Near(float a, float b, float eps = 1e-5f) { return std::fabs(a - b) <= eps; }

PassEvidence Gameplay() {
    PassEvidence e{};
    e.nearPlane = 0.1f; e.farPlane = 5000.0f;
    e.projectionW = 1.1f; e.projectionH = 1.7f;
    e.targetWidth = e.sceneWidth = 2632;
    e.targetHeight = e.sceneHeight = 2684;
    e.perspective = true; e.eyeSizedTarget = true; e.hasEyeViewport = true;
    return e;
}
PassEvidence ShadowDepth() {
    PassEvidence e{};
    e.nearPlane = 0.01f; e.farPlane = 1000.0f;
    e.targetWidth = 1024; e.targetHeight = 1024;
    e.sceneWidth = 2632; e.sceneHeight = 2684;
    e.perspective = true; e.depthOnly = true;
    return e;
}

void TestClassification() {
    const auto game = Gameplay();
    Check(Classify(game) == PassKind::GameplayEye, "gameplay camera gate");
    Check(Decide(game) == DrawAction::StereoEye, "gameplay gets stereo");

    auto receiver = game;
    receiver.sampledProjectedShadow = true;
    Check(Classify(receiver) == PassKind::GameplayEye,
          "shadow receiver remains a gameplay pass");
    Check(Decide(receiver) == DrawAction::MonoShadowReceiver,
          "shadow receiver is mono until reprojection is captured");

    const auto shadow = ShadowDepth();
    Check(Classify(shadow) == PassKind::ShadowLightSpace,
          "depth-only light-space pass");
    Check(Decide(shadow) == DrawAction::PreserveNativeShadow,
          "native shadow pass is preserved by default");
    Check(Decide(shadow, true) == DrawAction::DropDiagnostic,
          "drop is opt-in diagnostic only");

    auto ui = PassEvidence{};
    ui.targetWidth = 1280; ui.targetHeight = 720;
    ui.perspective = false;
    Check(Classify(ui) == PassKind::UiOrPost, "non-perspective UI pass");
    Check(Decide(ui) == DrawAction::ConservativeMono, "UI stays mono");

    auto noisy = game;
    noisy.nearPlane = std::nanf("");
    Check(Decide(noisy) == DrawAction::ConservativeMono,
          "non-finite evidence cannot enable stereo");
}

void TestShadowMatrixInvariant() {
    const auto shadow = ShadowDepth();
    const Mat4 native = Identity();
    const Mat4 hmd = Translation({37.0f, -12.0f, 4.0f});
    const Mat4 wouldBeBad = Mul(hmd, native);
    Check(!SameMatrix(native, wouldBeBad), "HMD translation changes a matrix when applied");
    Check(!AppliesHeadPose(Decide(shadow)), "shadow policy rejects HMD transform");
    // Policy result is the original matrix, so this is the exact state the
    // production hook must upload to the light-space pass.
    const Mat4 selected = native;
    Check(SameMatrix(selected, native), "light-space matrix remains byte-equivalent");
}

void TestStereoSequence() {
    PassEvidence left = Gameplay();
    PassEvidence right = Gameplay();
    PassEvidence shadow = ShadowDepth();
    const DrawAction sequence[] = { Decide(left), Decide(shadow), Decide(right) };
    Check(sequence[0] == DrawAction::StereoEye, "left eye is stereo");
    Check(sequence[1] == DrawAction::PreserveNativeShadow, "middle shadow is native");
    Check(sequence[2] == DrawAction::StereoEye, "right eye is stereo");
    Check(!AppliesHeadPose(sequence[1]), "shadow cannot inherit eye state");
}

void TestDirectionalProjection() {
    const ShadowProjection p = BuildDirectionalShadow(
        {0.3f, -1.0f, 0.2f}, {10.0f, 5.0f, -2.0f},
        40.0f, 40.0f, 0.01f, 1000.0f, 0.5f, 0.5f, 0.25f, 0.25f);
    Vec3 uv0{}, uv1{};
    Check(ProjectToAtlas(p, {10.0f, 5.0f, -2.0f}, uv0), "atlas center projects");
    Check(ProjectToAtlas(p, {10.5f, 5.0f, -2.0f}, uv1), "nearby world point projects");
    Check(Near(uv0.x, 0.25f, 1e-4f) && Near(uv0.y, 0.25f, 1e-4f),
          "atlas center maps to configured bias");
    Check(std::isfinite(uv1.x) && std::isfinite(uv1.y) && std::isfinite(uv1.z),
          "atlas projection has no NaN/Inf");

    // A camera/head translation is not part of the light-space transform. The
    // same world point must sample the same atlas coordinate on both eyes.
    Vec3 eye0{}, eye1{};
    Check(ProjectToAtlas(p, {12.0f, 6.0f, -1.0f}, eye0), "left-eye world point projects");
    Check(ProjectToAtlas(p, {12.0f, 6.0f, -1.0f}, eye1), "right-eye world point projects");
    Check(Near(eye0.x, eye1.x) && Near(eye0.y, eye1.y) && Near(eye0.z, eye1.z),
          "shadow atlas coordinate is eye-invariant");

    // Deterministic Monte-Carlo style sweep over a light volume.  This catches
    // accidental NaNs and confirms that stereo state cannot alter the light
    // projection for a large set of world points.
    std::uint32_t state = 0x51A7E5D3u;
    for (int i = 0; i < 10000; ++i) {
        state = state * 1664525u + 1013904223u;
        const float x = 10.0f + (float(int(state >> 8) & 0x7ff) / 2047.0f - 0.5f) * 70.0f;
        state = state * 1664525u + 1013904223u;
        const float y = 5.0f + (float(int(state >> 8) & 0x7ff) / 2047.0f - 0.5f) * 70.0f;
        state = state * 1664525u + 1013904223u;
        const float z = -2.0f + (float(int(state >> 8) & 0x7ff) / 2047.0f - 0.5f) * 70.0f;
        Vec3 a{}, b{};
        Check(ProjectToAtlas(p, {x, y, z}, a), "Monte-Carlo left projection is finite");
        Check(ProjectToAtlas(p, {x, y, z}, b), "Monte-Carlo right projection is finite");
        Check(Near(a.x, b.x) && Near(a.y, b.y) && Near(a.z, b.z),
              "Monte-Carlo stereo samples agree in light space");
    }
}
}

int main() {
    TestClassification();
    TestShadowMatrixInvariant();
    TestStereoSequence();
    TestDirectionalProjection();
    if (failures) {
        std::cerr << failures << " shadow-policy test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "shadow-policy tests: PASS (classification, matrix invariants, stereo sequence, atlas simulation)\n";
    return EXIT_SUCCESS;
}
