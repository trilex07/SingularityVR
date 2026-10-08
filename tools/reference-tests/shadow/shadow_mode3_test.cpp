// ShadowMode=3 tests: pass/register policy and the light-space compensation
// invariant. Pure math/logic - no D3D, no game data.
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstdint>

#include "../../../spikes/view_matrix/shadow_mode3.h"

using namespace singularity::shadow;

static int g_fail = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", msg); ++g_fail; }
}

static void TestDecision() {
    DrawEvidence shadowPass;                       // light-space generation
    shadowPass.layoutShadow = true; shadowPass.offscreenDepthOnly = true;
    shadowPass.isScene = false;
    Check(DecideDraw(shadowPass) == StereoAction::NativeShadowOnce,
          "shadow-map generation is drawn once, native");

    DrawEvidence scene;                            // normal receiver
    scene.isScene = true; scene.knownCameraSlot = true;
    Check(DecideDraw(scene) == StereoAction::StereoEye, "scene with a camera slot is stereo");

    DrawEvidence noCam;                            // camera slot went invalid
    noCam.isScene = true; noCam.knownCameraSlot = false;
    Check(DecideDraw(noCam) == StereoAction::MonoFallback, "no camera slot -> one unsplit draw");

    DrawEvidence bad; bad.finite = false;
    Check(DecideDraw(bad) == StereoAction::MonoFallback, "non-finite evidence never splits");

    DrawEvidence offscreen; offscreen.isScene = false;
    Check(DecideDraw(offscreen) == StereoAction::NativeShadowOnce, "non-scene target never splits");
}

static void TestRegisterPolicy() {
    Check(ShouldRemapRegister(true, false) == true, "camera matrix may be remapped");
    Check(ShouldRemapRegister(true, true) == false, "camera matrix that is also shadow stays native");
    Check(ShouldRemapRegister(false, false) == false, "non-camera is never remapped");

    ShadowRegisterCensus c;
    c.Mark(3); c.Mark(7);
    Check(c.IsShadow(3) && c.IsShadow(7), "census marks shadow slots");
    Check(!c.IsShadow(4), "unmarked slot is not shadow");
    Check(SlotMayRemap(c, 3, true) == false, "shadow slot never remapped");
    Check(SlotMayRemap(c, 5, true) == true, "non-shadow camera slot remapped");
    c.Clear();
    Check(c.Mask() == 0, "census clears");
}

static void TestCompensation() {
    // Build a plausible light view-projection (column-vector, M * p).
    const Mat4 L = BuildDirectionalShadow({-0.4f, -1.0f, 0.2f}, {0, 0, 0},
                                          60.0f, 60.0f, 1.0f, 400.0f).lightViewProjection;
    const Vec3 e{0.031f, 0.0f, 0.0f};  // ~real IPD half in game units

    Mat4 Lc{};
    Check(CompensateWorldOffset(L, e, Lc), "compensation is finite");

    // For any world point p, drawing it at (p + e) through Lc must project the
    // same as p through L - both eyes sample the identical shadow coordinate.
    const Vec3 pts[] = {{0,0,0},{5,2,-3},{-12,40,7},{100,-8,25},{0.5f,1.5f,-0.25f}};
    for (const Vec3& p : pts) {
        const Vec4 native = Transform(L, {p.x, p.y, p.z, 1.0f});
        const Vec4 shifted = Transform(Lc, {p.x + e.x, p.y + e.y, p.z + e.z, 1.0f});
        Check(std::fabs(native.x - shifted.x) < 1e-3f &&
              std::fabs(native.y - shifted.y) < 1e-3f &&
              std::fabs(native.z - shifted.z) < 1e-3f &&
              std::fabs(native.w - shifted.w) < 1e-3f,
              "compensated matrix keeps both eyes on the same shadow coordinate");
    }

    Mat4 out{};
    Check(CompensateWorldOffset(L, {std::nanf(""), 0, 0}, out) == false,
          "non-finite offset is rejected");
}

int main() {
    TestDecision();
    TestRegisterPolicy();
    TestCompensation();
    if (g_fail) { std::fprintf(stderr, "shadow-mode3 tests: %d FAILED\n", g_fail); return 1; }
    std::puts("PASS: ShadowMode=3 policy + light-space compensation invariants");
    return 0;
}
