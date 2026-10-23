#include "../ammo_layout.h"
#include <cmath>
#include <cstdio>

static int g_fails = 0;
static void Expect(bool cond, const char* name) {
    if (cond) { std::printf("PASS %s\n", name); return; }
    std::printf("FAIL %s\n", name);
    ++g_fails;
}

int main() {
    using namespace ammoplates;
    Expect(PlateForGunVerts(5799) == 0, "centurion plate");
    Expect(PlateForGunVerts(11387) == 1, "rifle plate");
    Expect(PlateForGunVerts(8297) == 2, "shotgun plate");
    Expect(PlateForGunVerts(11389) == 3, "sniper plate");
    Expect(PlateForGunVerts(0) == -1, "no gun no plate");
    Expect(PlateForGunVerts(99999) == -1, "unknown gun no plate");
    Expect(PlateFile(0) != nullptr && PlateFile(3) != nullptr, "plate files");
    Expect(PlateFile(9) == nullptr, "bad plate refused");
    Expect(EyePlateWidth(0.0f, 160, 1000) == 160, "unprojectable eye uses its own fallback");
    Expect(EyePlateWidth(INFINITY, 160, 1000) == 160, "nonfinite projection uses fallback");
    Expect(EyePlateWidth(1000.0f, 160, 1000) == 190, "metric width from current eye");
    Expect(EyePlateWidth(10000.0f, 160, 1000) == 300, "eye width bounded");
    Expect(EyePlateWidth(1e30f, 160, 1000) == 300, "finite extreme projection clamps before cast");
    // Glyph rects sit inside the 2172x240 strip, ordered, non-empty.
    {
        bool ok = true;
        int prev = -1;
        for (int d = 0; d < 10; ++d) {
            GlyphRect g = DigitGlyph(d);
            if (!(g.x0 >= 0 && g.x1 <= 2172 && g.y0 >= 0 && g.y1 <= 240)) ok = false;
            if (!(g.x1 > g.x0 && g.y1 > g.y0)) ok = false;
            if (!(g.x0 >= prev)) ok = false;
            prev = g.x0;
        }
        Expect(ok, "glyph rects sane");
    }
    // Covers sit inside the plate with positive area (red box measured
    // identical on all four plates).
    {
        float x0, y0, x1, y1, ok = true;
        MagCover(x0, y0, x1, y1);
        if (!(x0 >= 0 && x1 <= 1 && y0 >= 0 && y1 <= 1 && x1 > x0 && y1 > y0)) ok = false;
        ReserveCover(x0, y0, x1, y1);
        if (!(x0 >= 0 && x1 <= 1 && y0 >= 0 && y1 <= 1 && x1 > x0 && y1 > y0)) ok = false;
        WhiteMagCover(x0, y0, x1, y1);
        if (!(x0 >= 0 && x1 <= 1 && y0 >= 0 && y1 <= 1 && x1 > x0 && y1 > y0)) ok = false;
        RedBoxCover(x0, y0, x1, y1);
        if (!(x0 >= 0 && x1 <= 1 && y0 >= 0 && y1 <= 1 && x1 > x0 && y1 > y0)) ok = false;
        Expect(ok != 0, "cover rects sane");
    }
    // Reserve halo lock: baked "48" ends 1px from the old cover, so the rect
    // must enclose the glow (x<=0.495, y<=0.513, x>=0.606, y>=0.719).
    {
        float x0, y0, x1, y1;
        ReserveCover(x0, y0, x1, y1);
        Expect(x0 <= 0.495f && y0 <= 0.513f && x1 >= 0.606f && y1 >= 0.719f,
               "reserve halo margins");
    }
    // Pips band lock: true pips at x0.266-0.371/y0.616-0.692; rect must
    // enclose them without touching mag-icon bottom, corner tick (x<0.26)
    // or the y0.762 cell frame, and must clear live digits (bottom 0.594).
    {
        float x0, y0, x1, y1;
        MagPipsCover(x0, y0, x1, y1);
        Expect(x0 >= 0.255f && x0 <= 0.265f && x1 >= 0.375f && x1 <= 0.385f &&
               y0 >= 0.595f && y1 >= 0.695f && y1 <= 0.70f,
               "pips cover band");
    }
    Expect(MagEmpty(0) && MagEmpty(1) && MagEmpty(2) && !MagEmpty(3) && !MagEmpty(30), "empty rule");
    // Right-aligned layout ends exactly at rightX.
    {
        DigitQuad q[4];
        const float R = 890.0f, T = 260.0f, H = 170.0f;
        int n = LayoutNumber(30, 4, R, T, H, q, 4);
        Expect(n == 2, "two digits");
        if (n == 2) {
            Expect(q[0].glyph == 3 && q[1].glyph == 0, "digit order");
            Expect(std::fabs(q[1].x1 - R) < 0.01f, "right aligned");
            Expect(q[0].x0 < q[0].x1 && q[0].x1 <= q[1].x0 && q[1].x0 < q[1].x1, "no overlap");
            Expect(std::fabs((q[0].y1 - q[0].y0) - H) < 0.01f, "digit height");
        }
        n = LayoutNumber(0, 4, R, T, H, q, 4);
        Expect(n == 1 && q[0].glyph == 0, "zero draws");
        Expect(LayoutNumber(-5, 4, R, T, H, q, 4) == 1, "negative clamps");
        Expect(LayoutNumber(7, 0, R, T, H, q, 4) == 0, "no digits refused");
        Expect(LayoutNumber(7, 4, R, T, H, q, 0) == 0, "no capacity refused");
    }
    // Eye-half clamp: the whole quad must stay inside its own half, otherwise
    // it spills into the other eye and reads as a second peripheral copy.
    // Backbuffer 2000x1000, plate 160 wide: halves [0,1000) and [1000,2000).
    {
        const float W = 2000.0f, PW = 160.0f;
        Expect(ClampCxToEyeHalf(500.0f, PW, W, 0) == 500.0f, "centre untouched");
        Expect(ClampCxToEyeHalf(1500.0f, PW, W, 1) == 1500.0f, "centre untouched eye1");
        Expect(ClampCxToEyeHalf(950.0f, PW, W, 0) == 920.0f, "seam clamped left");
        Expect(ClampCxToEyeHalf(1050.0f, PW, W, 1) == 1080.0f, "seam clamped right");
        Expect(ClampCxToEyeHalf(-300.0f, PW, W, 0) == 80.0f, "offscreen clamped left edge");
        Expect(ClampCxToEyeHalf(2400.0f, PW, W, 1) == 1920.0f, "offscreen clamped right edge");
        Expect(ClampCxToEyeHalf(500.0f, PW, W, 1) == 1080.0f, "wrong-half anchor reined in");
        Expect(ClampCxToEyeHalf(1500.0f, PW, W, 0) == 920.0f, "wrong-half anchor reined in eye0");
        Expect(ClampCxToEyeHalf(500.0f, 1200.0f, W, 0) == 500.0f, "oversize plate centred");
        Expect(ClampCxToEyeHalf(2000.0f, 0.0f, W, 0) == 1000.0f, "zero width clamps to seam");
    }
    if (g_fails == 0) std::puts("ammo plates: layout L1");
    return g_fails == 0 ? 0 : 1;
}
