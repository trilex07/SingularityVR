#pragma once

// High-quality deferred decals (UE3): a fullscreen pass writes a scene-sized G16R16
// target (~2 draws/frame). It samples the depth buffer with ScreenPositionScaleBias-
// derived UVs. Stereo clip-space remapping without halving that constant shifts the
// read per eye — the square "shadow" mismatch. HQ decals OFF removes it (run 44).
// This header is pure logic for the per-eye PS constant fix (scope overlay precedent).

#include <cstdint>

namespace singularity::decal {

inline bool IsProjectionTargetFormat(std::uint32_t d3dFormat) {
    // D3DFMT_G16R16 = 34, D3DFMT_G16R16F = 112 in d3d9types.h
    return d3dFormat == 34u || d3dFormat == 112u;
}

inline bool IsSceneSizedTarget(std::uint32_t w, std::uint32_t h, std::uint32_t sceneW,
                               std::uint32_t sceneH) {
    return sceneW > 0 && sceneH > 0 && w == sceneW && h == sceneH;
}

// UE3 mad: u = (x/w)*c0 + c3, v = (y/w)*c1 + c2  (bias swizzle .wzzw on c1 register).
// Halve X scale and shift X bias so UV maps to one eye's half of a side-by-side depth buffer.
inline void RemapScreenPositionScaleBiasForEye(float inOut[4], int eye) {
    const float half = 0.5f;
    inOut[0] *= half;
    inOut[3] = inOut[3] * half + (eye ? half : 0.0f);
}

} // namespace singularity::decal
