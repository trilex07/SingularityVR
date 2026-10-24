// L1: diegetic weapon HUD math (Centurion counter). No D3D, no engine state.
// Runs from build/fixtures; the atlas PNG path is relative to that directory.
#include "../diegetic_weapon_hud.h"
#include <cstdio>
#include <cstdint>
#include <cmath>

static int g_fail = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %d %s\n", __LINE__, #c); ++g_fail; } } while (0)
static bool Near(float a, float b, float e) { return std::fabs(a - b) <= e; }

int main() {
    using namespace diegetichud;
    // 1. Measured mount dims (evidence: centurion fixture config, ff29efd).
    CHECK(Near(HousingWidthMetres(), 0.06604f, 1e-5f));
    CHECK(Near(HousingHeightMetres(), 0.01143f, 1e-5f));
    CHECK(LowState(0) && LowState(2) && !LowState(3));

    // 2. Atlas PNG header matches the hardcoded atlas size (no silent swap).
    FILE* f = fopen("../../assets/processed/hud/diegetic/diegetic_hud_atlas.png", "rb");
    CHECK(f != nullptr);
    if (f) {
        std::uint8_t h[24]; size_t r = fread(h, 1, 24, f); fclose(f);
        CHECK(r == 24 && h[12] == 'I' && h[13] == 'H' && h[14] == 'D'); // IHDR chunk
        const int w = (h[16] << 24) | (h[17] << 16) | (h[18] << 8) | h[19];
        const int hgt = (h[20] << 24) | (h[21] << 16) | (h[22] << 8) | h[23];
        CHECK(w == 1024 && hgt == 512);
    }

    // 3. Cell rows/geometry from the validated JSON.
    CHECK(ScreenBg().x == 264 && ScreenBg().y == 0 && ScreenBg().w == 256 && ScreenBg().h == 128);
    CHECK(Digit(false, 0).x == 106 && Digit(false, 0).y == 136);
    CHECK(Digit(false, 9).x == 106 + 82 * 9 && Digit(false, 9).y == 136);
    CHECK(Digit(true, 0).x == 106 && Digit(true, 0).y == 266);

    // 4. Gate: default OFF, and every listed condition can hide it. Unknown
    //    ammo and failed art must hide (never present guessed values).
    CHECK(PanelVisible(1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1));
    CHECK(!PanelVisible(0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1)); // default OFF
    CHECK(!PanelVisible(1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1)); // wheel open
    CHECK(!PanelVisible(1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1)); // paused
    CHECK(!PanelVisible(1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1)); // menu
    CHECK(!PanelVisible(1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1)); // panel
    CHECK(!PanelVisible(1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1)); // cinematic
    CHECK(!PanelVisible(1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1)); // not gameplay
    CHECK(!PanelVisible(1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1)); // not in hands
    CHECK(!PanelVisible(1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1)); // hand invalid
    CHECK(!PanelVisible(1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1)); // eyes missing
    CHECK(!PanelVisible(1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0)); // ammo unknown
    CHECK(!PanelVisible(1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1)); // art failed
    CHECK(!PanelVisible(1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1)); // other gun

    // 5. Sizing probe (world-anchored; explicit failures, not inherited).
    CHECK(PanelWidthPx(0.0f, 1080.0f) == 0.0f);
    CHECK(PanelWidthPx(-1.0f, 1080.0f) == 0.0f);
    CHECK(Near(PanelWidthPx(999999.0f, 1080.0f), 1080.0f * 0.12f, 1e-2f));
    CHECK(Near(PanelWidthPx(600.0f, 1080.0f), 0.06604f * 600.0f * 3.0f, 1e-3f));

    // 6. Layout: '06/24' => panel + 0,6 + slash + 2,4.
    Quad q[16]{}; const int n = LayoutCounter(6, 24, 300.0f, 200.0f, 200.0f, 100.0f, q, 16);
    CHECK(n == 6);
    CHECK(Near(q[0].x0, 200.0f, 1e-3f) && Near(q[0].x1, 400.0f, 1e-3f));
    CHECK(Near(q[0].y0, 150.0f, 1e-3f) && Near(q[0].y1, 250.0f, 1e-3f));
    CHECK(Near(q[0].u0, 264.0f / 1024.0f, 1e-6f) && Near(q[0].v1, 128.0f / 512.0f, 1e-6f));
    auto sameCell = [](const Quad& a, const Cell& c) {
        return Near(a.u0, c.x / AtlasW(), 1e-6f) && Near(a.v0, c.y / AtlasH(), 1e-6f);
    };
    CHECK(sameCell(q[1], Digit(false, 0)));  // leading zero of '06'
    CHECK(sameCell(q[2], Digit(false, 6)));
    CHECK(sameCell(q[3], Slash()));
    CHECK(sameCell(q[4], Digit(false, 2)));
    CHECK(sameCell(q[5], Digit(false, 4)));
    for (int i = 0; i < n; ++i) { CHECK(q[i].y0 < q[i].y1); CHECK(q[i].x0 < q[i].x1); }

    // 7. Low mag => amber glyph row for clip digits only.
    Quad lq[16]{}; CHECK(LayoutCounter(2, 24, 0, 0, 200, 100, lq, 16) == 6);
    CHECK(sameCell(lq[1], Digit(true, 0)) && sameCell(lq[2], Digit(true, 2)));
    CHECK(sameCell(q[4], Digit(false, 2)));

    // 8. Unknown values draw the panel without digits (runtime hides).
    Quad uq[16]{}; CHECK(LayoutCounter(-1, 24, 0, 0, 200, 100, uq, 16) == 1);

    // 9. Three-digit reserve keeps skipping ('120').
    Quad rq[16]{}; CHECK(LayoutCounter(6, 120, 0, 0, 200, 100, rq, 16) == 7);
    CHECK(sameCell(rq[4], Digit(false, 1)) && sameCell(rq[5], Digit(false, 2)) && sameCell(rq[6], Digit(false, 0)));

    // 10. Clamp keeps the panel inside one eye-half.
    float cx = 1.0f; ClampCentre(cx, 200.0f, 2000.0f); CHECK(Near(cx, 101.0f, 1e-3f));
    cx = 1999.0f; ClampCentre(cx, 200.0f, 2000.0f); CHECK(Near(cx, 1899.0f, 1e-3f));
    cx = std::nanf(""); ClampCentre(cx, 200.0f, 2000.0f); CHECK(Near(cx, 1000.0f, 1e-3f));

    printf(g_fail ? "test-diegetic-centurion: %d FAIL\n" : "test-diegetic-centurion: all L1 checks PASS\n", g_fail);
    return g_fail ? 1 : 0;
}
