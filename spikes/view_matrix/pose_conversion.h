#pragma once
#include <cmath>

namespace vrpose {
struct V3 { float x, y, z; };
struct Quat { float x, y, z, w; };
struct EulerYXZ { float pitchX, yawY, rollZ; };

// Preserve the production OpenXR {x,y,z,w} extraction convention used for
// both HMD and hand poses. This is the existing component extraction, not a
// claim that the dormant hand palette path currently applies wrist rotation.
inline EulerYXZ DecomposeOpenXrYXZ(Quat q) {
    float sinp = 2.0f * (q.w*q.x - q.y*q.z);
    if (sinp > 1.0f) sinp = 1.0f;
    if (sinp < -1.0f) sinp = -1.0f;
    return {std::asin(sinp),
            std::atan2(2.0f*(q.w*q.y+q.z*q.x), 1.0f-2.0f*(q.x*q.x+q.y*q.y)),
            std::atan2(2.0f*(q.w*q.z+q.x*q.y), 1.0f-2.0f*(q.x*q.x+q.z*q.z))};
}

// This is the position map already used by d3d9.cpp. OpenXR coordinates are
// +X right, +Y up, -Z forward. Singularity's translated-world coordinates
// are +X forward, +Y right, +Z up. Yaw angles use the existing XR-centre yaw
// followed by the game's base yaw; scale is applied exactly once here.
inline V3 TrackingDeltaToWorldUU(V3 metres, float centreYaw, float baseYaw,
                                float unitsPerMetre) {
    const float sc = std::sin(centreYaw), cc = std::cos(centreYaw);
    const float fwd = -metres.x * sc - metres.z * cc;
    const float right = metres.x * cc - metres.z * sc;
    const float bs = std::sin(baseYaw), bc = std::cos(baseYaw);
    return {(bc * fwd - bs * right) * unitsPerMetre,
            (bs * fwd + bc * right) * unitsPerMetre,
            metres.y * unitsPerMetre};
}

inline V3 Sub(V3 a, V3 b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
inline V3 Add(V3 a, V3 b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
inline float Length(V3 a) { return std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z); }
}
