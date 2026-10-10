#pragma once

// =============================================================================
// ShadowMode=3 : "protect the light-space matrix".
//
// Root cause (production d3d9.cpp, StereoPairBody ~L10992):
//   The stereo path does not translate geometry. It collects every constant
//   register that currently holds a camera matrix and re-uploads a PER-EYE
//   version of it (ApplyEyeRemap: clip-space half-width + eye offset). That is
//   correct for the camera view/projection. It is catastrophic for a
//   light-space (shadow) projection matrix that happens to land in the same
//   register set - the shadow lookup is then shifted/squashed per eye, which is
//   exactly the reported "blocky, flickering, one eye darker, different per
//   eye" behaviour. The production comment already names the register reuse:
//   "the same register is reused by shadow and post passes with different
//   [meaning]".
//
// The fix is a classification/policy, not new shadow rendering:
//   1. a light-space / shadow-map generation draw is drawn ONCE, native, and is
//      never split or remapped (this is the Dishonored-VR/DX9 rule: render
//      target identity beats a matrix-register hit; never split a shadow map);
//   2. a light-space projection register is NEVER included in the per-eye
//      camera remap, so both eyes sample the same shadow coordinates for the
//      same world point;
//   3. anything the policy cannot classify is drawn mono (never split), which
//      keeps the existing anti-flicker fallback.
//
// The register census is filled from live evidence (shader-declares-shadow, or
// a projected-shadow texture bound), never guessed. See docs/SHADOW_MODE3.md.
// =============================================================================

#include "shadow_pass_policy.h"

#include <cstdint>

namespace singularity::shadow {

// --- per-draw decision at the StereoPair seam -------------------------------

struct DrawEvidence {
    bool layoutShadow = false;        // shader declares ProjectionMatrix + DepthBias
    bool shadowMapBound = false;      // a projected-shadow texture is bound to this draw
    bool offscreenDepthOnly = false;  // drawing into the light-space shadow target
    bool isScene = true;              // true = scene/receiver target, false = offscreen
    bool knownCameraSlot = false;     // at least one live camera matrix register
    bool finite = true;               // near/far/matrices were all finite
};

// Named StereoAction to avoid colliding with shadow_pass_policy.h's DrawAction.
enum class StereoAction : std::uint8_t {
    StereoEye,         // per-eye camera remap, split per eye
    NativeShadowOnce,  // draw once, native, never split/remapped
    MonoFallback,      // unknown -> one unsplit draw (the anti-flicker fallback)
};

inline StereoAction DecideDraw(const DrawEvidence& e) {
    if (!e.finite) return StereoAction::MonoFallback;
    if (e.layoutShadow || e.offscreenDepthOnly) return StereoAction::NativeShadowOnce;
    if (!e.isScene) return StereoAction::NativeShadowOnce;
    if (e.knownCameraSlot) return StereoAction::StereoEye;
    return StereoAction::MonoFallback;
}

// The one-line fix, unit-tested: a shadow (light-space) matrix register is
// excluded from the per-eye camera remap.
inline bool ShouldRemapRegister(bool isCameraMatrix, bool isShadowMatrix) {
    return isCameraMatrix && !isShadowMatrix;
}

// --- live census of which constant slots hold a light-space matrix ----------
// Filled by the hook (never by this header). Bit s set => slot s is a shadow
// projection and must not be remapped.
class ShadowRegisterCensus {
public:
    void Clear() { mask_ = 0; }
    void Mark(std::uint32_t slot) { if (slot < 32) mask_ |= (1u << slot); }
    bool IsShadow(std::uint32_t slot) const {
        return slot < 32 && (mask_ & (1u << slot)) != 0;
    }
    std::uint32_t Mask() const { return mask_; }

private:
    std::uint32_t mask_ = 0;
};

// Decide whether a register slot may receive the per-eye upload.
// carryMask: light-space slots learned on the previous frame (see Present);
// protects early scene draws before this frame's shadow generation runs.
inline bool SlotMayRemap(const ShadowRegisterCensus& census, std::uint32_t slot,
                         bool isCameraMatrix, std::uint32_t carryMask = 0) {
    if (!isCameraMatrix) return false;
    if (census.IsShadow(slot)) return false;
    if (slot < 32 && (carryMask & (1u << slot)) != 0) return false;
    return true;
}

// --- optional: compensation if a future path offsets WORLD positions --------
// This mod remaps the camera clip matrices, so leaving the light matrix native
// is already exact. If a path ever offsets geometry in world space by `e`, the
// light matrix must be pre-compensated by the same world translation so both
// eyes still sample the native shadow:
//
//   column-vector convention (M * p):  L' = L * T(-e)
//   proof: L' * (p + e) = L * T(-e) * (p + e) = L * (p + e - e) = L * p
//
// Uses the Mat4/Vec3/Mul/Translation primitives from shadow_pass_policy.h.
inline bool CompensateWorldOffset(const Mat4& lightMatrix, Vec3 eyeOffsetWorldRedundant,
                                  Mat4& out) {
    // The argument is the geometry offset; we translate the light matrix the
    // other way so a point drawn at (p + e) projects as if it were at p.
    const Vec3 t{-eyeOffsetWorldRedundant.x, -eyeOffsetWorldRedundant.y,
                 -eyeOffsetWorldRedundant.z};
    for (float v : {t.x, t.y, t.z})
        if (!std::isfinite(v)) return false;
    out = Mul(lightMatrix, Translation(t));
    return true;
}

} // namespace singularity::shadow
