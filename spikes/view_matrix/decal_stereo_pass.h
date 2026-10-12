#pragma once

// HQ deferred decals: a small number of fullscreen draws per frame target a
// scene-sized G16R16 buffer. Only those draws may receive a per-eye fix.

#include <cmath>
#include <cstdint>

namespace singularity::decal {

inline bool IsProjectionTargetFormat(std::uint32_t d3dFormat) {
    return d3dFormat == 34u || d3dFormat == 112u; // G16R16 / G16R16F
}

inline bool IsSceneSizedTarget(std::uint32_t w, std::uint32_t h, std::uint32_t sceneW,
                               std::uint32_t sceneH) {
    return sceneW > 0 && sceneH > 0 && w == sceneW && h == sceneH;
}

inline bool VertexBoundsSpanNdc(float minX, float maxX, float minY, float maxY,
                                std::uint32_t vertCount) {
    if (vertCount < 3) return false;
    const float spanX = maxX - minX;
    const float spanY = maxY - minY;
    return spanX > 1.0f && spanY > 1.0f && minX > -1.5f && maxX < 1.5f && minY > -1.5f &&
           maxY < 1.5f;
}

inline bool MatchConfiguredPassShape(std::uint32_t primType, std::uint32_t primCount,
                                     std::uint32_t wantType, std::uint32_t wantCount) {
    return primType == wantType && primCount == wantCount;
}

inline bool LooksLikeScreenPositionScaleBias(const float c[4]) {
    return std::fabs(c[0] - 0.5f) < 0.08f && std::fabs(c[1] + 0.5f) < 0.08f;
}

inline void RemapScreenPositionScaleBiasForEye(float inOut[4], int eye) {
    const float half = 0.5f;
    inOut[0] *= half;
    inOut[3] = inOut[3] * half + (eye ? half : 0.0f);
}

} // namespace singularity::decal
