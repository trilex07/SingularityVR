#pragma once

// Procedural finger posing for the FP glove hands.
//
// The game drives the arm/wrist through its own animation plus the mod's IK
// deltas; fingers get no input-driven motion. This module adds VR-style finger
// curl (like other VR games): each finger curls 0..1 from controller input,
// smoothed, driving verified flexion axes baked from the bind skeleton
// (finger_table.h). Thumb, index, middle, ring, pinky per hand.
//
// Math (row-vector palette, v' = v * S, same as glove_isolate):
//   D = T(-c) * R(axis, +angle) * T(c), built in CURRENT animated space with
//   c = joint center (translation of the live skin matrix) and
//   axis = Rot(live skin) * bindAxis. Applied as S' = S * D to the joint and
//   all its descendants. Positive angle closes toward the palm for the baked
//   axis orientation (verified offline per finger); FLEX_SIGN flips globally if
//   a headset check ever disagrees.
//
// This header is pure logic. The main agent feeds it live palette matrices and
// OpenXR values; see docs/GLOVES_FINGERS.md for the seam.

#include "glove_isolate.h"  // Mat4, ApplyDelta (S*D), NameContains
#include "finger_table.h"

#include <cmath>
#include <cstring>

namespace fingerpose {

inline float FlexSign() { return 1.0f; }  // ini-tunable emergency flip

struct CurlInput {
    float index = 0.0f;    // trigger pull 0..1
    float middle = 0.0f;   // grip/squeeze 0..1
    float ring = 0.0f;     // grip/squeeze 0..1
    float pinky = 0.0f;    // grip/squeeze 0..1
    float thumb = 0.0f;    // 0 = resting on stick, 1 = curled in
};

struct CurlState {
    float index = 0.0f, middle = 0.0f, ring = 0.0f, pinky = 0.0f, thumb = 0.0f;
};

inline float Clamp01(float v) {
    if (!(v == v)) return 0.0f;
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

// Exponential approach: rate ~14/s, frame-rate independent, never overshoots.
inline void SmoothCurls(CurlState& s, const CurlInput& t, float dt) {
    if (!(dt > 0.0f)) return;
    if (dt > 0.25f) dt = 0.25f;
    const float k = dt * 14.0f > 1.0f ? 1.0f : dt * 14.0f;
    s.index += (Clamp01(t.index) - s.index) * k;
    s.middle += (Clamp01(t.middle) - s.middle) * k;
    s.ring += (Clamp01(t.ring) - s.ring) * k;
    s.pinky += (Clamp01(t.pinky) - s.pinky) * k;
    s.thumb += (Clamp01(t.thumb) - s.thumb) * k;
}

inline float CurlOf(const CurlState& s, const char* finger) {
    if (gloveisolate::NameContains(finger, "index")) return s.index;
    if (gloveisolate::NameContains(finger, "middle")) return s.middle;
    if (gloveisolate::NameContains(finger, "ring")) return s.ring;
    if (gloveisolate::NameContains(finger, "pinky")) return s.pinky;
    if (gloveisolate::NameContains(finger, "thumb")) return s.thumb;
    return 0.0f;
}

// Rotation about a unit axis, row-vector Rodrigues: v' = v*cos + (k x v)*sin
// + k*(k.v)*(1-cos). Identical formula to the offline closure verification,
// so the baked axis orientation (positive angle = closing) carries over.
inline void FlexDelta(const float c[3], const float axis[3], float angleRad,
                      gloveisolate::Mat4& out) {
    float len = std::sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
    float kx = 0.0f, ky = 0.0f, kz = 1.0f;
    if (len > 1e-6f) { kx = axis[0] / len; ky = axis[1] / len; kz = axis[2] / len; }
    const float co = std::cos(angleRad), si = std::sin(angleRad), ic = 1.0f - co;
    // R[r][c] = co*d + si*E + ic*k_r*k_c with (k x v)[c] = sum_r v[r]*E[r][c].
    float R[3][3];
    const float k[3] = {kx, ky, kz};
    // Explicit Levi-Civita contraction: E[r][c] = sum_k eps[k][r][c] * k[k].
    for (int r = 0; r < 3; ++r)
        for (int c2 = 0; c2 < 3; ++c2) {
            float e = 0.0f;
            for (int kk = 0; kk < 3; ++kk) {
                int e1 = (kk + 1) % 3, e2 = (kk + 2) % 3;
                if (e1 == r && e2 == c2) e += k[kk];
                if (e2 == r && e1 == c2) e -= k[kk];
            }
            R[r][c2] = co * (r == c2 ? 1.0f : 0.0f) + si * e + ic * k[r] * k[c2];
        }
    // D = T(-c) * R * T(c), row-vector affine (translation in row 3).
    for (int r = 0; r < 3; ++r)
        for (int c2 = 0; c2 < 3; ++c2) out.m[r][c2] = R[r][c2];
    out.m[0][3] = 0.0f; out.m[1][3] = 0.0f; out.m[2][3] = 0.0f;
    out.m[3][0] = c[0] - (R[0][0] * c[0] + R[1][0] * c[1] + R[2][0] * c[2]);
    out.m[3][1] = c[1] - (R[0][1] * c[0] + R[1][1] * c[1] + R[2][1] * c[2]);
    out.m[3][2] = c[2] - (R[0][2] * c[0] + R[1][2] * c[1] + R[2][2] * c[2]);
    out.m[3][3] = 1.0f;
}

// Reference bone record the main agent provides (names + parenting).
struct RefBone {
    const char* name;
    int parent;
};

inline int FindSlot(const unsigned* boneMap, unsigned slots, const RefBone* refs,
                    unsigned refCount, const char* boneName) {
    if (!boneMap || !refs || !boneName) return -1;
    for (unsigned s = 0; s < slots; ++s) {
        const unsigned b = boneMap[s];
        if (b < refCount && refs[b].name && std::strcmp(refs[b].name, boneName) == 0)
            return (int)s;
    }
    return -1;
}

inline bool IsDescendantOrSelf(unsigned bone, unsigned ancestor, const RefBone* refs,
                               unsigned refCount) {
    for (unsigned guard = 0; guard < 256 && bone < refCount; ++guard) {
        if (bone == ancestor) return true;
        const int p = refs[bone].parent;
        if (p < 0 || (unsigned)p >= refCount || (unsigned)p == bone) return false;
        bone = (unsigned)p;
    }
    return false;
}

// Build the input from OpenXR action states. Every source is optional:
// unavailable touch paths degrade to analog-only poses (documented below).
//   trigger01 : /user/hand/<side>/input/trigger/value (index)
//   grip01    : squeeze force if present else squeeze value (middle/ring/pinky)
//   stickTouch/thumbrestTouch/buttonTouch: any thumb contact (thumb rests)
// Thumb rule (other VR games): touching the stick = thumb parked (open);
// no contact = relaxed half curl; firm grip pulls it in.
inline void BuildInput(float trigger01, float grip01, bool hasTrigger, bool hasGrip,
                       bool stickTouch, bool restTouch, bool hasTouch, CurlInput& out) {
    const float trig = hasTrigger ? Clamp01(trigger01) : 0.0f;
    const float grip = hasGrip ? Clamp01(grip01) : 0.0f;
    out.index = trig;
    out.middle = grip;
    out.ring = grip;
    out.pinky = grip;
    if (!hasTouch) {
        out.thumb = 0.45f + 0.30f * grip;
    } else if (stickTouch || restTouch) {
        out.thumb = 0.12f + 0.25f * grip;
    } else {
        out.thumb = 0.45f + 0.30f * grip;
    }
}

}  // namespace fingerpose
