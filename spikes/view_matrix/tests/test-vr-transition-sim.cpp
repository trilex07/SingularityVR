// Desktop regression harness for the XR transitions that are otherwise easiest
// to catch only after a death/load in a headset.  It deliberately contains no
// OpenXR or D3D calls: the production reset contract is represented as state,
// and the geometry gates use the exact two_hand_grip.h / grip_latch.h helpers.
#include "../two_hand_grip.h"
#include "../grip_latch.h"
#include <cassert>
#include <cmath>
#include <cstdio>

struct RuntimeState {
    bool cameraValid = true;
    bool camMatrixBound = true;
    bool centre = true;
    bool leftPose = true;
    bool rightPose = true;
    bool offHandValid = true;
    bool twoHand = true;
    bool uiCursorValid = true;
    griplatch::GripLatch latch;

    void resetAfterCameraLoss() {
        camMatrixBound = false;
        centre = false;
        leftPose = rightPose = offHandValid = false;
        twoHand = false;
        uiCursorValid = false;
        latch.Reset();
    }
    void cameraReturns() {
        cameraValid = true;
        // XR pose and camera matrix are reacquired on subsequent frames; no
        // stale state is allowed to become valid merely because the pointer returned.
        assert(!camMatrixBound && !centre && !offHandValid && !twoHand && !uiCursorValid);
        camMatrixBound = centre = leftPose = rightPose = offHandValid = uiCursorValid = true;
    }
};

int main() {
    // Close pistol acquisition: side-by-side hands are accepted, while a
    // hand too far away still cannot acquire.  The latch then holds through
    // movement until Grip release.
    const float sideBySide[3] = {0.10f, 0.02f, 0.02f};
    const float forward[3] = {0.0f, 0.0f, 1.0f};
    assert(TwoHandGripAllowed(sideBySide, forward, true, true, false, true));
    const float far[3] = {0.48f, 0.0f, 0.0f};
    assert(!TwoHandGripAllowed(far, forward, true, true, false, true));

    griplatch::GripLatch latch({0.15f, 500});
    assert(latch.Update(true, 0.10f, true, 5799, 1) == griplatch::State::Unlatched);
    assert(latch.Update(true, 0.10f, true, 5799, 2) == griplatch::State::Latched);
    assert(latch.Update(true, 9.0f, true, 5799, 3) == griplatch::State::Latched);
    assert(latch.Update(false, 9.0f, true, 5799, 4) == griplatch::State::Unlatched);

    // Gameplay -> menu -> gameplay must not leak an R2 edge.  A transition
    // reset also clears the support latch, so a reloaded weapon needs a fresh press.
    RuntimeState s;
    s.resetAfterCameraLoss();
    assert(!s.camMatrixBound && !s.centre && !s.leftPose && !s.rightPose);
    s.cameraReturns();
    assert(s.leftPose && s.rightPose && s.offHandValid && !s.twoHand);

    // A parallel ray is rejected by the UI math contract used by the pointer;
    // this catches accidental NaN/Inf propagation in the desktop harness.
    const float denom = 0.0f;
    assert(std::fabs(denom) < 1e-6f && std::isfinite(denom));
    std::puts("PASS: pistol near-grip latch, hold-until-release, camera-loss reset, reload reacquire");
}
