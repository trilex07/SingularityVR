#pragma once

// =============================================================================
// Blood impact geometry (HOST side).
//
// A blood hit is a surface decal, not a camera-facing billboard: its quad must
// lie in the surface plane, aligned to the hit normal, so it stays projected
// correctly in both eyes. This mirrors the basis used by the plugin author's
// vfx_world_draw.h, kept here as the host's own tested module.
//
// The plugin already offsets the impact from the surface (position = hit +
// normal * epsilon) to avoid z-fighting; the host only builds the plane basis.
// =============================================================================

#include "vfx_plugin.h"

#include <cmath>

namespace singularity_vfx {

inline float VfxDot(const VfxVec3& a, const VfxVec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline VfxVec3 VfxCross(const VfxVec3& a, const VfxVec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline bool VfxNormalize(const VfxVec3& v, VfxVec3& out, float eps = 1e-8f) {
    const float len2 = VfxDot(v, v);
    if (!(len2 > eps)) return false;
    const float inv = 1.0f / std::sqrt(len2);
    out = {v.x * inv, v.y * inv, v.z * inv};
    return true;
}

// Builds a stable surface basis for a blood decal. `tangentHint` is normally the
// current eye's camera right vector; a deterministic fallback is used when the
// hit normal is parallel to that hint (looking straight down the surface normal).
inline bool BuildSurfaceBasis(const VfxVec3& normal, const VfxVec3& tangentHint,
                              VfxVec3& right, VfxVec3& up) {
    VfxVec3 n{};
    if (!VfxNormalize(normal, n)) return false;

    VfxVec3 tangent = VfxCross(tangentHint, n);
    if (!VfxNormalize(tangent, tangent)) {
        const VfxVec3 fallback = (std::fabs(n.y) < 0.9f) ? VfxVec3{0.0f, 1.0f, 0.0f}
                                                         : VfxVec3{1.0f, 0.0f, 0.0f};
        tangent = VfxCross(fallback, n);
        if (!VfxNormalize(tangent, tangent)) return false;
    }
    VfxVec3 bitangent = VfxCross(n, tangent);
    if (!VfxNormalize(bitangent, bitangent)) return false;

    right = tangent;
    up = bitangent;
    return true;
}

} // namespace singularity_vfx
// probe
