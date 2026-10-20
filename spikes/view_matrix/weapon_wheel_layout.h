#pragma once
// MAIN: round weapon-wheel screen-space geometry + label sanitation.
// Pure math, no D3D and no engine state, so it is L1-testable in isolation.
// Screen space: +x right, +y DOWN. Sector 0 is straight up, sectors proceed
// clockwise — the same convention as weaponwheel::SectorForStick and the
// existing square-card placement (-pi/2 + 2*pi*i/slots in cos/sin space).
//
// Reference mockup (references/95f791be-....png): circular 6-wedge wheel with
// per-sector labels/icons, an orange highlight on the selected wedge and a
// centre disc. Ammo counts from the mockup are deliberately OUT of scope:
// no native ammo hook has been proven (AMMO_UNKNOWN), so this module never
// fabricates counts.
#include <cmath>
#include <cstddef>
#include <cstring>

#include "ui_pointer.h"
#include "weapon_wheel.h"

namespace wheel_layout {

inline float Pi() { return 3.14159265358979323846f; }

// Centre direction of sector i in "stick space": c = atan2(sx, -sy).
// Screen point at radius r: x = cx + sin(c)*r, y = cy - cos(c)*r.
inline float SectorCenter(int i, int slots) {
    if (slots < 2) slots = 2;
    if (slots > 8) slots = 8;
    if (i < 0) i = 0;
    if (i >= slots) i = slots - 1;
    return 2.0f * Pi() * (float)i / (float)slots;
}

inline float SectorSpan(int slots) {
    if (slots < 2) slots = 2;
    if (slots > 8) slots = 8;
    return 2.0f * Pi() / (float)slots;
}

// Wedge angular bounds with a visual separation gap (radians, split evenly on
// both sides). A non-positive gap yields contiguous wedges. The gap is clamped
// so at least 20% of the span remains drawn.
inline void WedgeBounds(int i, int slots, float gapRad, float& a0, float& a1) {
    const float half = SectorSpan(slots) * 0.5f;
    float gap = gapRad * 0.5f;
    const float maxGap = half * 0.8f;
    if (gap < 0.0f) gap = 0.0f;
    if (gap > maxGap) gap = maxGap;
    const float c = SectorCenter(i, slots);
    a0 = c - half + gap;
    a1 = c + half - gap;
}

inline void WedgePoint(float cx, float cy, float r, float c, float& x, float& y) {
    x = cx + sinf(c) * r;
    y = cy - cosf(c) * r;
}

// Pixel metrics derived from the backbuffer height only, so both eyes agree.
// Wheel diameter is ~40% of the frame height: readable labels without covering
// the scene. All values are ints; no per-frame allocation needed downstream.
struct Metrics {
    int outerR = 0;    // wedge outer radius
    int innerR = 0;    // centre disc radius
    int iconHalf = 0;  // half-size of the square icon quad
    int labelR = 0;    // radius of the label anchor
    int iconR = 0;     // radius of the icon anchor
    int textPx = 2;    // glyph pixel size for sector labels + centre text
    int ringSteps = 64;
    int wedgeSteps = 6;
};

inline Metrics RoundMetrics(int backbufferH) {
    Metrics m;
    if (backbufferH < 8) backbufferH = 8;
    m.outerR = backbufferH * 16 / 100;
    if (m.outerR < 80) m.outerR = 80;
    m.innerR = m.outerR * 36 / 100;
    m.iconHalf = m.outerR * 17 / 100;
    if (m.iconHalf < 12) m.iconHalf = 12;
    m.labelR = m.outerR * 80 / 100;
    m.iconR = m.outerR * 52 / 100;
    m.textPx = 2;
    m.ringSteps = 64;
    m.wedgeSteps = 6;
    return m;
}

// A hand near a shoulder/hip can project outside the eye. Keep the complete
// wheel in the central view instead of clipping acquired sectors offscreen.
// Selection still uses the unchanged hand/stick directions, not this centre.
inline float VisibleCenter(float projected, int extent, int radius) {
    if (extent <= 0 || !std::isfinite(projected)) return 0.5f;
    const float margin = (float)(radius+8)/(float)extent;
    if (margin >= 0.5f) return 0.5f;
    const float lo = margin > 0.35f ? margin : 0.35f;
    const float hi = 1.0f-lo;
    return projected < lo ? lo : (projected > hi ? hi : projected);
}

// Label sanitation for the 5x7 glyph renderer (kFont in d3d9.cpp): it knows
// A-Z (case-insensitive), 0-9, space and + - ( ). Anything else renders as a
// silent SPACE, which is indistinguishable from a missing character — so the
// mapping is made explicit here and covered by tests. Unsupported bytes
// (including UTF-8 multibyte sequences for Polish diacritics) become one
// space each; callers may collapse runs if desired. Returns chars written
// excluding the terminator. Destination is always terminated when cap > 0.
inline int SanitizeLabel(const char* src, char* dst, size_t cap) {
    if (cap == 0) return 0;
    if (!src) { dst[0] = 0; return 0; }
    size_t n = 0;
    for (const unsigned char* p = (const unsigned char*)src; *p; ++p) {
        unsigned char ch = *p;
        char out;
        if (ch >= 'a' && ch <= 'z') out = (char)(ch - 'a' + 'A');
        else if ((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')) out = (char)ch;
        else if (ch == ' ' || ch == '+' || ch == '-' || ch == '(' || ch == ')') out = (char)ch;
        else out = ' ';
        if (n + 1 < cap) dst[n++] = out;
        else break;
    }
    dst[n] = 0;
    return (int)n;
}

// Largest label the wheel will draw (glyphs are wide in VR).
enum { kMaxLabel = 16 };

// Pick the display label for a wheel slot.
// Wheel positions map 1:1 to catalog gun sectors (see GunOrder), so every
// position is a real weapon: no slot needs a "current" placeholder override.
// Ownership gate: a sector whose gun_verts is positive, is not the currently
// held weapon, and is absent from the seen set (weapons the player has
// verifiably held this session) yields NO label — the wheel never names a
// weapon the player does not own yet. seen may be null (gate skipped).
// Returns false when the slot has no usable label (caller draws the wedge
// empty: icon and label both skipped). The FSM is untouched: an untouched
// release still cancels via the touched flag (no commit pulses).
inline bool LabelForSlot(const char* const* labels, const long* verts, int n,
                         int slot, long currentVerts, const long* seen, int nSeen,
                         char* dst, size_t cap) {
    if (!labels || !verts || n <= 0 || !dst || cap == 0) return false;
    if (slot < 0 || slot >= n) return false;
    const char* pick = labels[slot];
    const long pickVerts = verts[slot];
    if (!pick || !pick[0]) return false;
    if (pickVerts > 0 && pickVerts != currentVerts) {
        bool owned = false;
        for (int i = 0; seen && i < nSeen; ++i) {
            if (seen[i] == pickVerts) { owned = true; break; }
        }
        if (!owned) return false;
    }
    char clean[kMaxLabel + 1];
    SanitizeLabel(pick, clean, sizeof(clean));
    // Reject labels that sanitise to blank (e.g. "--" is fine, "   " is not).
    bool any = false;
    for (const char* p = clean; *p; ++p) {
        if (*p != ' ') { any = true; break; }
    }
    if (!any) return false;
    // Copy with hard truncation at kMaxLabel display glyphs.
    size_t ni = 0;
    for (const char* p = clean; *p && ni + 1 < cap && ni < (size_t)kMaxLabel; ++p) dst[ni++] = *p;
    dst[ni] = 0;
    return ni > 0;
}

// Hand-driven selection: the hand offset D (meters, head-relative tracking
// space: (hand - head) - anchor, where anchor was latched at wheel open).
// The offset is read as a screen-space direction through the CURRENT eye
// basis, so turning the head with a still hand does not steer the highlight.
// Returns -2 when no reading is possible (caller falls back to the stick),
// -1 inside the deadzone (keep current highlight), else 0..slots-1.
enum { kHandSlotsMax = 8 };
inline float HandDeadzoneM() { return 0.06f; }   // 6 cm of hand travel
inline float HandNdcDeadzone() { return 0.02f; } // guards the forward push

struct EyeBasis {
    uipointer::V pos;   // eye position (tracking space)
    uipointer::V fwd;   // view forward
    uipointer::V right; // view right
    uipointer::V up;    // view up
    float tanHalfX = 0; // symmetric half-FOV tangents
    float tanHalfY = 0;
};

inline int HandSector(float dx, float dy, float dz, const EyeBasis& eye, int slots) {
    slots = weaponwheel::ClampSlots(slots);
    const float m2 = dx * dx + dy * dy + dz * dz;
    const float dead = HandDeadzoneM();
    if (!(m2 >= dead * dead)) return -1; // zero/NaN/sub-deadzone travel
    if (eye.tanHalfX <= 0 || eye.tanHalfY <= 0) return -2;
    // Read D as a screen-space direction: project a reference point 2 m ahead
    // of the eye and the same point shifted by D. A push straight along the
    // view axis yields no direction (-1); a point behind the eye fails (-2).
    const uipointer::V ref{ eye.pos.x + eye.fwd.x * 2.0f, eye.pos.y + eye.fwd.y * 2.0f,
                            eye.pos.z + eye.fwd.z * 2.0f };
    const uipointer::V tgt{ ref.x + dx, ref.y + dy, ref.z + dz };
    uipointer::ScreenPoint a, b;
    if (!uipointer::ProjectPoint(ref, eye.pos, eye.fwd, eye.right, eye.up,
                                 eye.tanHalfX, eye.tanHalfY, a))
        return -2;
    if (!uipointer::ProjectPoint(tgt, eye.pos, eye.fwd, eye.right, eye.up,
                                 eye.tanHalfX, eye.tanHalfY, b))
        return -2; // target behind the eye: no meaningful direction
    if (a.depth <= 0 || b.depth <= 0) return -2;
    const float du = b.u - a.u, dv = b.v - a.v; // u right, v DOWN (screen-like)
    if (!(du * du + dv * dv >= HandNdcDeadzone() * HandNdcDeadzone())) return -1;
    return weaponwheel::SectorForStick(du, dv, slots, 0.0f);
}

// Wheel positions map to catalog sectors that name a real weapon
// (gun_verts > 0), in catalog order: with the shipped catalog, positions
// 0..3 are Centurion / Valkyrie / Spikeshot / Kasimov and every sector is
// committable (an untouched release still cancels via the touched flag).
// Returns positions filled. A short catalog yields fewer; the caller falls
// back to identity order so the wheel never shows fewer sectors than slots.
inline int GunOrder(const long* verts, int n, int* out, int cap) {
    if (!verts || !out || cap <= 0) return 0;
    int m = 0;
    for (int i = 0; i < n && m < cap; ++i) {
        if (verts[i] > 0) out[m++] = i;
    }
    return m;
}

} // namespace wheel_layout
