#pragma once
// VFX muzzle placement: pure geometry, no engine state, L1-testable.
//
// The fire event carries a muzzle point, but GunPivotWorld is the weapon's
// ROTATION point (grip area), not the barrel tip. The effect therefore needs
// two corrections on top of the pivot:
//   FORWARD along the aim dir (barrel length varies per gun: 150 mm is about
//     right for a pistol, far too short for a rifle) -> per-gun table below.
//   UP in world units (the bore sits above the grip; without it the effect
//     lands UNDER the gun line) -> global [Render] VfxMuzzleUpMm INI knob
//     (default 0 = today's rendering), tuned live without a rebuild.
//
// Barrel lengths below are STARTING VALUES for L3 in-headset A/B, not
// measurements: the pivot is exact, the per-gun lengths are not yet measured.
// Unknown guns fall back to the global VfxMuzzleOffsetMm knob.
namespace vfxmuzzle {

// Forward barrel length ahead of the pivot, millimetres, by held-gun verts
// (same ids as ammoplates::PlateForGunVerts).
inline int ForwardMmForVerts(long verts, int fallbackMm) {
    switch (verts) {
        case 5799:  return 150;   // Centurion pistol: current default, keep
        case 11387: return 300;   // AR9 Valkyrie: STARTING VALUE, verify L3
        case 8297:  return 200;   // Spikeshot: STARTING VALUE, verify L3
        case 11389: return 400;   // Kasimov SNV-E99: STARTING VALUE, verify L3
        default:    return fallbackMm;
    }
}

// INI bound for VfxMuzzleUpMm (mirrors the 0..200 step-50 forward knob).
inline int ClampUpMm(int v) { return v < 0 ? 0 : (v > 200 ? 200 : v); }

} // namespace vfxmuzzle
