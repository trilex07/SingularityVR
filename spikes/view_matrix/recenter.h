#pragma once
#include <cmath>

// Stage-space yaw recenter for SingularityVR.
//
// Requirement (research brief section 7): RIGHT OPTIONS hold recenters VR so
// that ONE tracking-space transform moves the HMD, left controller and right
// controller consistently.
//
// Reference behaviour (Dishonored-VR, branch VR-Main @ 7c1cb8a32):
//   - src/core/vr/openxr_runtime.cpp:2624-2633 keeps the XR runtime on
//     LOCAL space and watches its generation; game code in
//     src/game/dishonored/head_track.cpp:58-62 (RecenterHead),
//     head_track.cpp:1455-1477 (positional ref + yaw-frame projection) and
//     src/core/vr/mono_anchor.h:10-26 (yaw-only upright panel parking)
//     recenters the GAME reference, never the runtime origin.
// See docs/REFERENCE_VR_IMPLEMENTATIONS.md. This header is an original
// implementation of the same tri-lane idea (no copied code):
//   1. The runtime origin is never touched here.
//   2. Recenter computes ONE yaw-only stage correction from the current head
//      pose; Apply() carries HMD + both controllers through it, so their
//      relative geometry is exactly preserved.
//   3. Yaw-only (no pitch/roll, no vertical shift): the floor stays the
//      floor, matching Dishonored's yaw-only panel parking.
//
// Conventions: Y-up, metres, yaw about +Y with forward = -Z when yaw = 0.
// forward(yaw) = (sin(yaw), 0, -cos(yaw)). Row-vector application:
// world = stage rotated by yaw then translated.

namespace vrrecenter {

struct V {
    float x = 0, y = 0, z = 0;
};
inline V operator+(V a, V b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline V operator-(V a, V b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline bool Finite(V v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }

// Yaw-only stage correction: rotate stage points about Y by `yaw`, then add
// the ground-plane translation (tx, tz). Height passes through untouched.
struct Stage {
    float yaw = 0; // radians
    float tx = 0, tz = 0; // metres, world ground plane
};

inline V Apply(const Stage& s, V p) {
    // Rotation convention matches Forward(): R(a) * Forward(0) = Forward(a).
    const float c = std::cos(s.yaw), q = std::sin(s.yaw);
    return V{p.x * c - p.z * q + s.tx, p.y, p.x * q + p.z * c + s.tz};
}

inline V Forward(float yaw) { return {std::sin(yaw), 0, -std::cos(yaw)}; }

// Build the stage correction that places the head at the world origin
// facing -Z (yaw 0) while preserving every controller's pose relative to
// the head. headYaw is the head's current yaw in stage space.
inline Stage RecenterForHead(V headPos, float headYaw) {
    Stage s;
    if (!Finite(headPos) || !std::isfinite(headYaw)) return s;
    s.yaw = -headYaw;
    const V moved = Apply(s, headPos);
    s.tx = -moved.x;
    s.tz = -moved.z;
    return s;
}

// Yaw of a facing direction: inverse of Forward(). Pitch/roll in the input
// are ignored by construction (uses the ground-plane projection).
inline float YawOf(V forward) {
    const float n = std::sqrt(forward.x * forward.x + forward.z * forward.z);
    if (!(n > 1e-6f)) return 0;
    return std::atan2(forward.x / n, -forward.z / n);
}

} // namespace vrrecenter
