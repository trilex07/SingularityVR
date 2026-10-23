#pragma once
// MAIN: left-glove health-bar show logic. Pure math, no D3D and no engine
// state, so it is L1-testable in isolation. Units are meters in tracking
// space; the caller owns hand/head poses, the two-hand latch and HP values.
//
// The bar appears only when the left hand is raised toward the face.
// Hysteresis keeps it from flickering; lowering the hand also hides it.
#include <cmath>
#include <cstdint>
#include <cstring>

#include "ui_pointer.h"

namespace healthbar {

// Mockup (references/Futurystyczny pasek zdrowia HUD.png): 11 segment slots,
// 10 filled + 1 empty frame. Filled count scales with hp/max.
inline int SegmentSlots() { return 11; }

// Show/hide radii around the head for the face-raise gesture.
inline float ShowRadiusM() { return 0.50f; }
inline float HideRadiusM() { return 0.60f; }
constexpr unsigned kBackgroundAlphaPct=65;
constexpr unsigned kSegmentAlphaPct=80;
inline int PlateWidth(int renderHeight) { return renderHeight>0?(int)std::fmax(1.f,renderHeight*.12f):0; }

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
    if (!(hy > py - 0.22f)) return false;                   // below face
    return true;
}

// True when a shown bar should disappear: hand left the outer radius or moved
// behind the face plane.
inline bool FaceRaiseHide(float hx, float hy, float hz, float px, float py, float pz,
                          float fx, float fy, float fz) {
    const float r = HideRadiusM();
    if (Dist2(hx, hy, hz, px, py, pz) > r * r) return true;
    if (!(hy > py - 0.30f)) return true; // lowered hand must not retain the latch
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

// Pawn.Health/HealthMax are IEEE-754 floats in memory (e.g. 0x43100000 =
// 144.0); reading the bits as int32 keeps ordering (positive floats compare
// equal as ints, which is all the heal window needs) but destroys ratios, so
// a segment bar built from int reads is stuck near-full. Reinterpret, never
// cast: memcpy keeps aliasing rules intact.
inline float HpFromBits(std::int32_t bits) {
    float f = 0.0f;
    std::memcpy(&f, &bits, sizeof(f));
    return f;
}

// One authoritative, read-only decoding step for every VR health display.
// The legacy wrist strip once divided the raw IEEE-754 bit patterns as ints;
// that can look almost full even at half health. The ten-cell value is for a
// future TMD-mounted readout; the current Katorga plate keeps its 11 slots.
struct LiveHealth {
    float current = 0.0f;
    float maximum = 0.0f;
    float fraction = 0.0f;
    int cells10 = -1;
    int plateSlots = -1;
    bool valid = false;
};

inline LiveHealth DecodeLiveHealth(std::int32_t currentBits, std::int32_t maximumBits) {
    LiveHealth result;
    result.current = HpFromBits(currentBits);
    result.maximum = HpFromBits(maximumBits);
    if (!std::isfinite(result.current) || !std::isfinite(result.maximum) ||
        result.current < 0.0f || result.maximum <= 0.0f) return result;
    result.fraction = std::fmin(1.0f, result.current / result.maximum);
    result.cells10 = (int)std::lround(result.fraction * 10.0f);
    result.plateSlots = FilledSegments(result.current, result.maximum);
    result.valid = result.plateSlots >= 0;
    return result;
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
