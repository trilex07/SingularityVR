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
    // Covers sit inside the plate with positive area.
    {
        float x0, y0, x1, y1, ok = true;
        MagCover(x0, y0, x1, y1);
        if (!(x0 >= 0 && x1 <= 1 && y0 >= 0 && y1 <= 1 && x1 > x0 && y1 > y0)) ok = false;
        ReserveCover(x0, y0, x1, y1);
        if (!(x0 >= 0 && x1 <= 1 && y0 >= 0 && y1 <= 1 && x1 > x0 && y1 > y0)) ok = false;
        WhiteMagCover(x0, y0, x1, y1);
        if (!(x0 >= 0 && x1 <= 1 && y0 >= 0 && y1 <= 1 && x1 > x0 && y1 > y0)) ok = false;
        Expect(ok != 0, "cover rects sane");
    }
    Expect(MagEmpty(0) && !MagEmpty(1) && !MagEmpty(30), "empty rule");
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
    if (g_fails == 0) std::puts("ammo plates: layout L1");
    return g_fails == 0 ? 0 : 1;
}
