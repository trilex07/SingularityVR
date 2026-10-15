#pragma once
// MAIN: left-glove health-bar show logic. Pure math, no D3D and no engine
// state, so it is L1-testable in isolation. Units are meters in tracking
// space; the caller owns hand/head poses, the two-hand latch and HP values.
//
// The bar appears in two situations: the left hand is raised toward the face
// to look at it, or the pistol is held two-handed (the bar then rides above
// the left hand). Hysteresis keeps it from flickering at the boundary.
#include <cmath>

#include "ui_pointer.h"

namespace healthbar {

// Mockup (references/Futurystyczny pasek zdrowia HUD.png): 11 segment slots,
// 10 filled + 1 empty frame. Filled count scales with hp/max.
inline int SegmentSlots() { return 11; }

// Show/hide radii around the head for the face-raise gesture.
inline float ShowRadiusM() { return 0.40f; }
inline float HideRadiusM() { return 0.55f; }

inline float Dist2(float ax, float ay, float az, float bx, float by, float bz) {
    const float dx = ax - bx, dy = ay - by, dz = az - bz;
    return dx * dx + dy * dy + dz * dz;
}

// True when a hidden bar should appear: hand close to the head, in front of
// the face, and not hanging at the waist.
inline bool FaceRaiseShow(float hx, float hy, float hz, float px, float py, float pz,
                          float fx, float fy, float fz) {
    const float r = ShowRadiusM();
    if (!(Dist2(hx, hy, hz, px, py, pz) < r * r)) return false;
    const float dx = hx - px, dy = hy - py, dz = hz - pz;
    if (!(dx * fx + dy * fy + dz * fz > 0.0f)) return false; // behind the face
    if (!(hy > py - 0.35f)) return false;                   // hanging low
    return true;
}

// True when a shown bar should disappear: hand left the outer radius or moved
// behind the face plane.
inline bool FaceRaiseHide(float hx, float hy, float hz, float px, float py, float pz,
                          float fx, float fy, float fz) {
    const float r = HideRadiusM();
    if (Dist2(hx, hy, hz, px, py, pz) > r * r) return true;
    const float dx = hx - px, dy = hy - py, dz = hz - pz;
    if (!(dx * fx + dy * fy + dz * fz > 0.0f)) return true;
    return false;
}

// Filled segment count for hp/max over SegmentSlots(). Returns -1 when the
// reading is unusable (max <= 0, NaN, or negative hp): the caller must not
// present a fabricated bar. hp >= max yields a full bar.
inline int FilledSegments(float hp, float maxHp) {
    const int n = SegmentSlots();
    if (!(maxHp > 0.0f)) return -1;
    if (!(hp >= 0.0f)) return -1;
    if (hp >= maxHp) return n;
    return (int)std::lround(hp * (float)n / maxHp);
}

// Slot quad fractions of the plate, measured from the mockup PNG
// (plate 2001x330 at (84,183); slot pitch 142.9 px; frames y 246..500).
// Slot i spans [x0,x1]x[y0,y1] in plate UV space. Indices clamp.
inline void SlotFrac(int i, float& x0, float& y0, float& x1, float& y1) {
    if (i < 0) i = 0;
    if (i >= SegmentSlots()) i = SegmentSlots() - 1;
    x0 = 0.1589f + (float)i * 0.0714f;
    x1 = x0 + 0.0715f;
    y0 = 0.191f;
    y1 = 0.961f;
}

} // namespace healthbar
