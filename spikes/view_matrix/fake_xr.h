#pragma once
#include <cmath>
#include <cstdint>

// Deterministic fake tracked-pose harness for SingularityVR.
//
// Motivation (from Dishonored-VR, branch VR-Main @ 7c1cb8a32):
//   - src/tools/xrsim/xrsim_*.cpp: 32-bit mock OpenXR runtime with scriptable
//     head/hand motion, recenter, FOV, capture/layers and pacing, driven by
//     scripts such as mono.xrs / stereo.xrs / world-6dof.xrs.
//   - src/core/vr/pose_record.cpp/.h: CAMERA vs RENDER vs SUBMISSION split
//     records with generation counters for diagnosing wrong-eye/tag bugs.
//   - src/core/vr/hud_anchor.h + tools/hud-anchor-tests.cpp: placement math
//     kept free of OpenXR handles so host tests fail it before any headset.
// See docs/REFERENCE_VR_IMPLEMENTATIONS.md. This file is an original
// implementation of the same concept for our headers (no copied code).
//
// Purpose: test hand transforms, menu projection, laser intersection,
// recenter and arm IK without wearing the headset. All motion is a closed
// form function of the frame index (no RNG, no wall clock), so two rigs with
// the same seed produce bit-comparable streams and frame stepping is
// exactly replayable.

namespace fakexr {

struct Vec {
    float x = 0, y = 0, z = 0;
};
struct Quat {
    float x = 0, y = 0, z = 0, w = 1;
};
struct Pose {
    Vec position; // metres, Y-up, -Z forward (OpenXR stage space)
    Quat orientation;
    bool positionTracked = true;
    bool orientationTracked = true;
};
inline bool PoseValid(const Pose& p) { return p.positionTracked && p.orientationTracked; }

struct Frame {
    Pose hmd, left, right;
    std::uint64_t index = 0;
    double time = 0; // seconds at a fixed 90 Hz step
};

// Closed-form yaw-only head orientation (upright, no pitch/roll).
inline Quat YawQuat(float yawRadians) {
    const float h = yawRadians * 0.5f;
    Quat q;
    q.x = 0;
    q.y = std::sin(h);
    q.z = 0;
    q.w = std::cos(h);
    return q;
}

class Rig {
public:
    explicit Rig(std::uint32_t seed = 1) : seed_(seed ? seed : 1) { Reset(); }
    void Reset() {
        index_ = 0;
        headYaw_ = 0;
        leftDrop_ = 0;
        rightDrop_ = 0;
        leftNudge_[0] = leftNudge_[1] = leftNudge_[2] = 0;
        rightNudge_[0] = rightNudge_[1] = rightNudge_[2] = 0;
    }
    // Constant head-turn rate (rad/s) for sweep tests. 0 = still.
    void SetHeadTurnRate(float radiansPerSecond) { turnRate_ = radiansPerSecond; }
    // Fixed positional offset added to a controller (metres, stage space).
    void NudgeController(bool right, float dx, float dy, float dz) {
        float* n = right ? rightNudge_ : leftNudge_;
        n[0] = dx;
        n[1] = dy;
        n[2] = dz;
    }
    // Simulate tracking loss for the next N frames (poses flagged invalid,
    // positions frozen at last valid -- mirrors runtime hold behaviour).
    void DropTracking(bool right, unsigned frames) {
        if (right)
            rightDrop_ = frames;
        else
            leftDrop_ = frames;
    }
    Frame Step(float dt = 1.0f / 90.0f) {
        Frame f;
        f.index = index_;
        f.time = static_cast<double>(index_) * dt;
        headYaw_ += turnRate_ * dt;
        // Seeded per-hand sway: deterministic sinusoids, ~2 cm / ~5 deg.
        const float s = static_cast<float>(seed_);
        const float t = static_cast<float>(f.time);
        f.hmd.position = Vec{0.02f * std::sin(0.9f * t + s), 1.62f + 0.01f * std::sin(1.7f * t),
                             -0.05f * std::sin(0.6f * t + 0.5f * s)};
        f.hmd.orientation = YawQuat(headYaw_);
        f.hmd.positionTracked = f.hmd.orientationTracked = true;
        const float swayL = std::sin(2.1f * t + s), swayR = std::sin(2.3f * t + 2.0f * s);
        f.left.position = Vec{-0.22f + leftNudge_[0] + 0.01f * swayL, 1.30f + leftNudge_[1],
                              -0.35f + leftNudge_[2] + 0.008f * swayR};
        f.left.orientation = YawQuat(headYaw_ + 0.15f * swayL);
        f.right.position = Vec{0.22f + rightNudge_[0] + 0.01f * swayR, 1.30f + rightNudge_[1],
                               -0.35f + rightNudge_[2] + 0.008f * swayL};
        f.right.orientation = YawQuat(headYaw_ + 0.15f * swayR);
        f.left.positionTracked = f.left.orientationTracked = leftDrop_ == 0;
        f.right.positionTracked = f.right.orientationTracked = rightDrop_ == 0;
        if (leftDrop_ > 0) --leftDrop_;
        if (rightDrop_ > 0) --rightDrop_;
        ++index_;
        return f;
    }
    std::uint64_t Index() const { return index_; }

private:
    std::uint32_t seed_;
    std::uint64_t index_ = 0;
    float turnRate_ = 0;
    float headYaw_ = 0;
    float leftNudge_[3], rightNudge_[3];
    unsigned leftDrop_ = 0, rightDrop_ = 0;
};

} // namespace fakexr
