#pragma once

// Glove isolation: hide the stiff forearm/upper-arm flesh and sleeve, keep the
// gloves (wrist subtree) and the TMD device.
//
// Method: per-bone collapse. Every palette entry whose bone is in the collapse
// set is replaced by D * S, where D = T(w) * Scale(s) * T(-w) collapses space
// toward the CURRENT wrist center w. Forearm-weighted vertices shrink toward the
// wrist; wrist/finger-weighted vertices are untouched.
// No index-buffer surgery, so no dependence on runtime triangle order.
//
// Kept by design (user decision): the TMD device bones (tmd_base, hinge,
// rail, disc, nixie, faceplate, wheel, nubs) stay native - the device floats
// where the forearm was, gloves stay on both hands.
//
// This header is pure logic + name tables. The main agent feeds it the live
// palette (original + output arrays, section bone map, wrist centers) from the
// existing arm-draw seam. Modes: 0 = off, 1 = census/log-only, 2 = gloves on controllers
// (hide forearm draws; no palette collapse). 3 = legacy wrist collapse (stretchy; avoid).

#include <cstring>

namespace gloveisolate {

// The pistol foreground pass contains one separately submitted 390-vertex
// sleeve/finger fragment (first index 14490, 674 triangles). In the
// gloves-only path this is not part of the wrist subtree: collapsing its
// palette leaves a detached finger-shaped island in the world. Keep the
// signature here so the render guard and the L1 test share the same narrow,
// engine-observed rule instead of a vertex-count-only heuristic.
inline bool IsStrayFingerSection(unsigned verts, unsigned first, unsigned triangles) {
    return (verts == 390u || verts == 3314u) && first == 14490u && triangles == 674u;
}

// Skinned forearm section (7704 tris @ first 13206). Hidden in gloves-only mode instead
// of collapsing palette entries (collapse stretches blended vertices).
inline bool IsArmFleshSection(unsigned verts, unsigned first, unsigned triangles) {
    return (verts == 7444u || verts == 4818u || verts == 4781u) && first == 13206u &&
           triangles == 7704u;
}

// Hands-only rendering: gloves stay on the controllers, the long forearm section
// is not drawn, and elbow IK must not rewrite the palette. Vertices must move
// rigidly with the controller delta, not with a wrist-centered stump scale.
inline bool HandsOnly(int showControllerArms, int isolateMode) {
    return showControllerArms == 0 && isolateMode >= 2;
}

// Mode 2 hides arm draws only. Mode 3 is the legacy palette stump (stretchy).
inline bool UsesPaletteStump(int showControllerArms, int isolateMode) {
    return showControllerArms == 0 && isolateMode >= 3;
}

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

// Return which of the two independent controller deltas a model-side bone
// consumes.  The result is deliberately expressed in delta-space rather than
// physical-side space: 0 is the off-hand delta and 1 is the aim-hand delta.
// This keeps LeftHanded mode correct without duplicating a second l/r table in
// the D3D9 draw hook.
inline int ControllerDeltaForBone(const char* name, int offHand) {
    const int side = HandSide(name);
    if ((side != 0 && side != 1) || (offHand != 0 && offHand != 1)) return -1;
    return side == offHand ? 0 : 1;
}

// Palette convention is row-vector (translation in row 3, v' = v * S), same as
// the mod's Mat. A post-transform D composes as S' = S * D, i.e.
// out[r][c] = sum_k s[r][k] * d[k][c].
//
// Collapse delta D = T(-w) * Scale(s) * T(w): diagonal s, row 3 = w*(1-s).
// For a rigid S, every skinned vertex maps to s*v + (1-s)*w.
//
// keepScale MUST stay above zero. s = 0 collapses the forearm onto a single
// point and kills every triangle touching it (730/4402 dead tris on the real
// tmd mesh: over-cut wrist rim + see-through palm). s = 0.12 keeps a small
// stump around the wrist that plugs the opening: 0 dead tris, wrist rim
// intact (verified on the real PSK weights). s = 1 is identity (no cut).
// Tune via ini (GloveStumpScale).
struct Mat4 { float m[4][4]; };

inline void CollapseDelta(const float w[3], Mat4& out, float keepScale = 0.12f) {
    float s = keepScale;
    if (!(s >= 0.0f)) s = 0.0f;   // NaN -> legacy point collapse
    if (s > 1.0f) s = 1.0f;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c) out.m[r][c] = 0.0f;
    out.m[0][0] = s; out.m[1][1] = s; out.m[2][2] = s;
    out.m[3][0] = w[0] * (1.0f - s);
    out.m[3][1] = w[1] * (1.0f - s);
    out.m[3][2] = w[2] * (1.0f - s);
    out.m[3][3] = 1.0f;
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

// Exported pistol-glove section palette (3314 verts @ first 0). Must include
// every bone that skinning weights reference; missing entries stay on native pose.
inline constexpr unsigned kNoTmdGlovePaletteBoneCount = 45;
inline constexpr unsigned kNoTmdGlovePalette[kNoTmdGlovePaletteBoneCount] = {
    85, 51, 70, 71, 72, 79, 80, 76, 73, 74, 82, 83, 77, 84, 81, 78, 75, 50,
    6, 7, 8, 9, 10, 11, 12, 16, 27, 23, 24, 20, 17, 18, 14, 15, 21, 25, 22, 19,
    41, 42, 43, 44, 45, 46, 47,
};

inline bool NoTmdGlovePaletteCovers(unsigned boneIndex) {
    for (unsigned b : kNoTmdGlovePalette)
        if (b == boneIndex) return true;
    return false;
}

}  // namespace gloveisolate
