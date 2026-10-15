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

// Cover quads (plate fractions): baked digit bands. The mag cover extends to
// y1=0.71 to also hide the baked cyan segment pips under the mag digits
// (measured bright y210-236 on all four plates); the live digits sit above
// them. The reserve bullets icon bottoms (y178) dip ~3px into the widened
// reserve cover top (y174.6); that fringe is dim icon glow, accepted to fully
// enclose the digits halo.
inline void MagCover(float& x0, float& y0, float& x1, float& y1) {
    x0 = 0.265f; y0 = 0.345f; x1 = 0.416f; y1 = 0.710f;
}
// Reserve digits ghost fix (user 2026-10-15: burnt "48" outline still
// visible): the baked glyphs end 1px from the old cover (right x616 vs 617,
// top y179 vs 179) so their glow halo bled past. Widened to enclose the
// halo; bullets icon bottoms stay above y0 (gap rows y172-179 are the
// dimmest on all four plates).
inline void ReserveCover(float& x0, float& y0, float& x1, float& y1) {
    x0 = 0.494f; y0 = 0.512f; x1 = 0.606f; y1 = 0.720f;
}
inline void WhiteMagCover(float& x0, float& y0, float& x1, float& y1) {
    x0 = 0.095f; y0 = 0.330f; x1 = 0.210f; y1 = 0.620f;
}
// Red warning box (mag + `!`): shown ONLY when the mag is empty, covered
// otherwise. Measured identical on all four plates.
inline void RedBoxCover(float& x0, float& y0, float& x1, float& y1) {
    x0 = 0.650f; y0 = 0.422f; x1 = 0.876f; y1 = 0.699f;
}
inline unsigned CoverColor() { return 0xFF041A2E; } // measured plate bg (4,26,46), opaque

// Baked segment-pip row under the mag digits: true pips live at x272-380 /
// y210-236 on all four plates (column profile; the x202-224 content is the
// mag icon bottom + corner tick, the y260 line is the cell frame — both
// deco the user flagged red). Tight rect covers pips only.
inline void MagPipsCover(float& x0, float& y0, float& x1, float& y1) {
    x0 = 0.260f; y0 = 0.600f; x1 = 0.380f; y1 = 0.698f;
}

// Digit zones: right edge + top + digit height (plate fractions).
inline void MagDigits(float& rightEdge, float& top, float& height) {
    rightEdge = 0.410f; top = 0.359f; height = 0.235f;
}
inline void ReserveDigits(float& rightEdge, float& top, float& height) {
    rightEdge = 0.594f; top = 0.539f; height = 0.152f;
}
inline float DigitGapFrac() { return 0.08f; } // × digit display height

// Empty rule (user 2026-10-15): full white mag icon while clip > 2, red
// warning box once 2 or fewer rounds remain. The white mag cell itself is
// never covered: the left mag icon stays visible in both states.
inline bool MagEmpty(int clip) { return clip <= 2; }

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
