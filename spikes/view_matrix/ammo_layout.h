#pragma once
// MAIN: Katorga ammo-plate layout. Pure math, no D3D and no engine state, so
// it is L1-testable. All fractions are of the plate quad (plate art is
// 2172x724 per gun; digits strip is 2172x240 cropped at y240-480).
//
// Per held gun the plate draws semi-transparent behind the gun: bg quad,
// cover quads over the baked digit bands, live digit glyphs right-aligned,
// and (when the mag is empty) a cover over the white mag cell. Hidden while
// the R3 wheel is open so the two never overlap.
namespace ammoplates {

// Plate pick by first-person gun verts (catalog ids).
inline int PlateForGunVerts(long verts) {
    if (verts == 5799) return 0;   // Centurion
    if (verts == 11387) return 1;  // AR9 Valkyrie
    if (verts == 8297) return 2;   // Spikeshot
    if (verts == 11389) return 3;  // Kasimov SNV-E99
    return -1;                     // TMD / seeker / unknown: no plate
}

inline const char* PlateFile(int plate) {
    switch (plate) {
        case 0: return "plate-centurion.png";
        case 1: return "plate-rifle.png";
        case 2: return "plate-shotgun.png";
        case 3: return "plate-sniper.png";
        default: return nullptr;
    }
}

// Digit glyph strip widths in strip px (glyph height ~170 px).
inline int DigitStripW(int d) {
    static const int w[10] = { 178, 72, 174, 176, 184, 176, 178, 162, 182, 174 };
    if (d < 0 || d > 9) d = 0;
    return w[d];
}

// Digit glyph rect in ammo-digits.png (2172x240).
struct GlyphRect { int x0, y0, x1, y1; };
inline GlyphRect DigitGlyph(int d) {
    static const int x0[10] = { 55, 287, 429, 643, 865, 1089, 1301, 1517, 1723, 1939 };
    static const int x1[10] = { 239, 365, 609, 825, 1055, 1271, 1485, 1685, 1911, 2119 };
    if (d < 0 || d > 9) d = 0;
    return GlyphRect{ x0[d], 35, x1[d], 205 };
}

// Cover quads (plate fractions): baked digit bands + white-mag cell.
inline void MagCover(float& x0, float& y0, float& x1, float& y1) {
    x0 = 0.265f; y0 = 0.345f; x1 = 0.416f; y1 = 0.615f;
}
inline void ReserveCover(float& x0, float& y0, float& x1, float& y1) {
    x0 = 0.502f; y0 = 0.525f; x1 = 0.603f; y1 = 0.704f;
}
inline void WhiteMagCover(float& x0, float& y0, float& x1, float& y1) {
    x0 = 0.095f; y0 = 0.330f; x1 = 0.210f; y1 = 0.620f;
}
inline unsigned CoverColor() { return 0xFF041A2E; } // measured plate bg (4,26,46), opaque

// Digit zones: right edge + top + digit height (plate fractions).
inline void MagDigits(float& rightEdge, float& top, float& height) {
    rightEdge = 0.410f; top = 0.359f; height = 0.235f;
}
inline void ReserveDigits(float& rightEdge, float& top, float& height) {
    rightEdge = 0.594f; top = 0.539f; height = 0.152f;
}
inline float DigitGapFrac() { return 0.08f; } // × digit display height

// Empty rule: white mag cell covered iff the mag is empty (red box + red
// mag stay as the mockup draws them).
inline bool MagEmpty(int clip) { return clip <= 0; }

// Right-aligned number layout in plate px. Returns digit quads (glyph ids +
// rects); value clamps to [0, 9999]. Returns 0 and leaves output untouched
// when maxDigits <= 0 or cap <= 0.
struct DigitQuad { float x0, y0, x1, y1; int glyph; };
inline int LayoutNumber(int value, int maxDigits, float rightX, float topY, float digitH,
                        DigitQuad* out, int cap) {
    if (maxDigits <= 0 || cap <= 0 || !out) return 0;
    if (value < 0) value = 0;
    if (value > 9999) value = 9999;
    int digits[4];
    int n = 0;
    do {
        digits[n++] = value % 10;
        value /= 10;
    } while (value > 0 && n < 4);
    if (n > maxDigits) n = maxDigits; // keep least significant (values fit in practice)
    const float scale = digitH / 170.0f;
    const float gap = DigitGapFrac() * digitH;
    float total = 0.0f;
    for (int i = 0; i < n; ++i) {
        total += (float)DigitStripW(digits[n - 1 - i]) * scale;
        if (i > 0) total += gap;
    }
    float x = rightX - total;
    int m = 0;
    for (int i = 0; i < n && m < cap; ++i) {
        const int g = digits[n - 1 - i];
        const float w = (float)DigitStripW(g) * scale;
        out[m].x0 = x;
        out[m].y0 = topY;
        out[m].x1 = x + w;
        out[m].y1 = topY + digitH;
        out[m].glyph = g;
        ++m;
        x += w + gap;
    }
    return m;
}

} // namespace ammoplates
