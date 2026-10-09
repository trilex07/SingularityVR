#pragma once

// Glove isolation: hide the stiff forearm/upper-arm flesh and sleeve, keep the
// gloves (wrist subtree) and the TMD device.
//
// Method: per-bone collapse. Every palette entry whose bone is in the collapse
// set is replaced by D * S, where D = T(w) * Scale(0) * T(-w) collapses space
// onto the CURRENT wrist center w. Forearm-weighted vertices (611 in tmd mat0,
// 992 in noTMD mat0, counted from the PSK weights) shrink to the wrist point;
// wrist/finger-weighted vertices are untouched; boundary triangles degenerate.
// No index-buffer surgery, so no dependence on runtime triangle order.
//
// Kept by design (user decision): the TMD device bones (tmd_base, hinge,
// rail, disc, nixie, faceplate, wheel, nubs) stay native - the device floats
// where the forearm was, gloves stay on both hands.
//
// This header is pure logic + name tables. The main agent feeds it the live
// palette (original + output arrays, section bone map, wrist centers) from the
// existing arm-draw seam. Modes: 0 = off, 1 = census/log-only, 2 = apply.

#include <cstring>

namespace gloveisolate {

inline bool NameContains(const char* name, const char* part) {
    if (!name || !part) return false;
    for (const char* h = name; *h; ++h) {
        const char* a = h; const char* b = part;
        while (*a && *b && *a == *b) { ++a; ++b; }
        if (!*b) return true;
    }
    return false;
}

// Wrist subtree (gloves): wrist + all finger bones, either hand.
inline bool IsWristSubtree(const char* name) {
    return NameContains(name, "_wrist") || NameContains(name, "_thumb_") ||
           NameContains(name, "_index_") || NameContains(name, "_middle_") ||
           NameContains(name, "_ring_") || NameContains(name, "_pinky");
}

// TMD device rigid parts (kept): children of l_arm_forearm that are hardware.
inline bool IsTmdDevice(const char* name) {
    return NameContains(name, "tmd_base") || NameContains(name, "hinge") ||
           NameContains(name, "rail") || NameContains(name, "disc") ||
           NameContains(name, "nixie") || NameContains(name, "faceplate") ||
           NameContains(name, "wheel") || NameContains(name, "nub_");
}

// Utility bones carry no skinning weight in the FP meshes; collapsing them
// would be harmless but keeping them is clearer in a census.
inline bool IsUtility(const char* name) {
    return NameContains(name, "Root") || NameContains(name, "IK_") ||
           NameContains(name, "cog_") || NameContains(name, "inview_") ||
           NameContains(name, "Spine_") || NameContains(name, "camera") ||
           NameContains(name, "sound");
}

// Arm flesh + sleeve: forearm chains, upper arm, shoulder girdle, sleeve fixups.
// sleeve_fix_* have no l_/r_ prefix (all parented under l_arm_forearm).
inline bool IsArmFlesh(const char* name) {
    return NameContains(name, "forearm") || NameContains(name, "elbow") ||
           NameContains(name, "bicep") || NameContains(name, "shoulder") ||
           NameContains(name, "clavicle") || NameContains(name, "sleeve_fix");
}

enum class BoneRole { Keep, Collapse, Review };

// Review = matched nothing: census-logged, never collapsed (fail-safe).
inline BoneRole Classify(const char* name) {
    if (!name || !*name) return BoneRole::Review;
    if (IsWristSubtree(name) || IsTmdDevice(name) || IsUtility(name)) return BoneRole::Keep;
    if (IsArmFlesh(name)) return BoneRole::Collapse;
    return BoneRole::Review;
}

inline int HandSide(const char* name) {
    if (!name) return -1;
    if (name[0] == 'l' && name[1] == '_') return 0;
    if (name[0] == 'r' && name[1] == '_') return 1;
    if (NameContains(name, "sleeve_fix")) return 0;  // all under l_arm_forearm
    return -1;
}

// Palette convention is row-vector (translation in row 3, v' = v * S), same as
// the mod's Mat. A post-transform D composes as S' = S * D, i.e.
// out[r][c] = sum_k s[r][k] * d[k][c].
//
// Collapse delta D = T(-w) * Scale(0) * T(w): row 3 = (w, 1), rest 0.
// For a rigid S, S' has rows 0..2 = 0 and row 3 = (w, 1), so every skinned
// vertex maps exactly onto the wrist center w (boundary triangles degenerate).
struct Mat4 { float m[4][4]; };

inline void CollapseDelta(const float w[3], Mat4& out) {
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c) out.m[r][c] = 0.0f;
    out.m[3][0] = w[0]; out.m[3][1] = w[1]; out.m[3][2] = w[2]; out.m[3][3] = 1.0f;
}

inline void ApplyDelta(const float s[4][4], const Mat4& d, float out[4][4]) {
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            out[r][c] = s[r][0] * d.m[0][c] + s[r][1] * d.m[1][c] +
                        s[r][2] * d.m[2][c] + s[r][3] * d.m[3][c];
}

inline void TransformPoint(const float m[4][4], const float p[3], float out[3]) {
    out[0] = m[0][0] * p[0] + m[1][0] * p[1] + m[2][0] * p[2] + m[3][0];
    out[1] = m[0][1] * p[0] + m[1][1] * p[1] + m[2][1] * p[2] + m[3][1];
    out[2] = m[0][2] * p[0] + m[1][2] * p[1] + m[2][2] * p[2] + m[3][2];
}

}  // namespace gloveisolate
