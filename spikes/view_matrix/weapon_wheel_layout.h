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
    m.outerR = backbufferH * 20 / 100;
    if (m.outerR < 96) m.outerR = 96;
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
// sectors: parallel arrays of catalog labels (may be null/empty).
// slot0ShowsCurrent: when true and currentVerts matches a labelled catalog
// entry, slot 0 (the no-op "current" sector) shows that weapon's label so the
// top wedge reads as the held weapon instead of a cryptic placeholder.
// Returns false when the slot has no usable label (caller draws icon only).
// The FSM is untouched: slot 0 release is still Cancel (no commit pulses).
inline bool LabelForSlot(const char* const* labels, const long* verts, int n,
                         int slot, long currentVerts, char* dst, size_t cap) {
    if (!labels || !verts || n <= 0 || !dst || cap == 0) return false;
    if (slot < 0 || slot >= n) return false;
    const char* pick = labels[slot];
    if (slot == 0 && currentVerts > 0) {
        for (int i = 0; i < n; ++i) {
            if (verts[i] == currentVerts && labels[i] && labels[i][0]) { pick = labels[i]; break; }
        }
    }
    if (!pick || !pick[0]) return false;
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

} // namespace wheel_layout
