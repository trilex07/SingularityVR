#include "../diegetic_hud.h"
#include <cstdio>
#include <cmath>
#include <cassert>

static int g_fails = 0;
static void Expect(bool cond, const char* name) {
    if (cond) {
        std::printf("PASS %s\n", name);
    } else {
        std::printf("FAIL %s\n", name);
        ++g_fails;
    }
}

int main() {
    using namespace diegetichud;
    std::printf("=== Running Diegetic HUD Tests ===\n");

    // 1. Atlas UV Rects Validation
    {
        bool ok = true;
        for (int d = 0; d < 10; ++d) {
            AtlasRect r = DigitRect(d);
            if (r.u0 < 0.0f || r.u1 > 1.0f || r.v0 < 0.0f || r.v1 > 1.0f) ok = false;
            if (r.u1 <= r.u0 || r.v1 <= r.v0) ok = false;
        }
        AtlasRect slash = SlashRect();
        if (slash.u0 < 0.0f || slash.u1 > 1.0f || slash.v1 <= slash.v0) ok = false;
        AtlasRect bezel = BezelBgRect();
        if (bezel.u0 < 0.0f || bezel.u1 > 1.0f || bezel.v1 <= bezel.v0) ok = false;
        AtlasRect screen = ScreenBgRect();
        if (screen.u0 < 0.0f || screen.u1 > 1.0f || screen.v1 <= screen.v0) ok = false;
        AtlasRect segFull = HealthSegFullRect();
        if (segFull.u0 < 0.0f || segFull.u1 > 1.0f || segFull.v1 <= segFull.v0) ok = false;
        AtlasRect segEmpty = HealthSegEmptyRect();
        if (segEmpty.u0 < 0.0f || segEmpty.u1 > 1.0f || segEmpty.v1 <= segEmpty.v0) ok = false;

        Expect(ok, "atlas UV rects valid and inside [0,1]");
    }

    // 2. Weapon Mount Profiles
    {
        WeaponMountProfile pCent = GetWeaponProfile(5799);
        Expect(pCent.widthUU > 1.0f && pCent.widthUU < 3.0f, "centurion width sane");
        Expect(pCent.heightUU > 0.5f && pCent.heightUU < 2.0f, "centurion height sane");
        Expect(pCent.pitchDeg < 0.0f, "centurion tilts back towards shooter");

        WeaponMountProfile pValk = GetWeaponProfile(11387);
        Expect(pValk.localY > pCent.localY, "valkyrie receiver higher than centurion");
        Expect(pValk.widthUU > 1.0f, "valkyrie width sane");

        WeaponMountProfile pShot = GetWeaponProfile(8297);
        Expect(pShot.localZ > 1.0f, "spikeshot mounted forward on receiver");

        WeaponMountProfile pSnip = GetWeaponProfile(11389);
        Expect(pSnip.localY > 2.0f, "kasimov mounted cleanly under optics");
    }

    // 3. Wrist Mount Profile
    {
        WristMountProfile pWrist = GetWristProfile();
        Expect(pWrist.lengthUU > 1.5f && pWrist.lengthUU < 4.0f, "wrist gauge length sane");
        Expect(pWrist.widthUU > 0.4f && pWrist.widthUU < 1.5f, "wrist gauge width sane");
        Expect(pWrist.rollDeg > 20.0f && pWrist.rollDeg < 60.0f, "wrist roll faces inward");
    }

    // 4. Color Dynamics
    {
        // Ammo
        Expect((AmmoColor(30) & 0x00FFFFFF) == 0x0000E5FF, "full ammo is E99 cyan");
        Expect((AmmoColor(2) & 0x00FFFFFF) == 0x00FF9100, "2 rounds left is amber warning");
        Expect((AmmoColor(1) & 0x00FFFFFF) == 0x00FF9100, "1 round left is amber warning");
        Expect((AmmoColor(0) & 0x00FFFFFF) == 0x00FF1744, "empty clip is crimson alert");

        // Health
        Expect((HealthColor(1.0f) & 0x00FFFFFF) == 0x0000E5FF, "100% HP is E99 cyan");
        Expect((HealthColor(0.75f) & 0x00FFFFFF) == 0x0000E5FF, "75% HP is E99 cyan");
        Expect((HealthColor(0.50f) & 0x00FFFFFF) == 0x00FF9100, "50% HP is amber warning");
        Expect((HealthColor(0.25f) & 0x00FFFFFF) == 0x00FF1744, "25% HP is crimson alert");
    }

    // 5. Layout Ammo Quads
    {
        Quad3D quads[20];
        // Standard Valkyrie ammo: 30 clip, 120 bin
        int n = LayoutAmmoQuads(11387, 30, 120, quads, 20);
        Expect(n >= 7, "valkyrie 30/120 generates full quad set");

        // Check quad properties: bezel (0), screen (1), icon (2), slash (3), clip digits, bin digits
        bool allFinite = true;
        for (int i = 0; i < n; ++i) {
            for (int v = 0; v < 4; ++v) {
                if (!std::isfinite(quads[i].v[v].x) ||
                    !std::isfinite(quads[i].v[v].y) ||
                    !std::isfinite(quads[i].v[v].z)) allFinite = false;
            }
        }
        Expect(allFinite, "all ammo quad vertices finite");

        // Check empty clip layout
        int nEmpty = LayoutAmmoQuads(5799, 0, 18, quads, 20);
        Expect(nEmpty >= 5, "empty clip generates quads");
        // Check that digits are rendered with crimson color
        Expect((quads[4].v[0].color & 0x00FFFFFF) == 0x00FF1744, "empty clip digit has crimson color");
    }

    // 6. Layout Health Quads
    {
        Quad3D quads[20];
        // 100% Health
        int n100 = LayoutHealthQuads(100.0f, 100.0f, quads, 20);
        // Bezel (0) + Screen (1) + Icon (2) + 10 Segments = 13 quads
        Expect(n100 == 13, "100% health generates exactly 13 quads");
        // All 10 segments filled
        bool allFilled = true;
        for (int i = 3; i < 13; ++i) {
            if ((quads[i].v[0].color & 0x00FFFFFF) != 0x0000E5FF) allFilled = false;
        }
        Expect(allFilled, "all 10 segments illuminated cyan at 100% HP");

        // 50% Health
        int n50 = LayoutHealthQuads(50.0f, 100.0f, quads, 20);
        Expect(n50 == 13, "50% health generates 13 quads");
        int filledCount = 0;
        for (int i = 3; i < 13; ++i) {
            if ((quads[i].v[0].color & 0x00FFFFFF) == 0x00FF9100) filledCount++;
        }
        Expect(filledCount == 5, "5 segments illuminated amber at 50% HP");

        // 20% Health (Critical)
        int n20 = LayoutHealthQuads(20.0f, 100.0f, quads, 20);
        Expect(n20 == 13, "20% health generates 13 quads");
        int critCount = 0;
        for (int i = 3; i < 13; ++i) {
            if ((quads[i].v[0].color & 0x00FFFFFF) == 0x00FF1744) critCount++;
        }
        Expect(critCount == 2, "2 segments illuminated crimson at 20% HP");
    }

    // 7. 3D Matrix Transformation
    {
        float M[4][4] = {
            { 1, 0, 0, 0 },
            { 0, 1, 0, 0 },
            { 0, 0, 1, 0 },
            { 10.0f, 20.0f, 30.0f, 1.0f } // Translation
        };
        float pt[3] = { 1.0f, 2.0f, 3.0f };
        float res[3];
        TransformPoint(pt, M, res);
        Expect(res[0] == 11.0f && res[1] == 22.0f && res[2] == 33.0f, "translation transform exact");

        Quad3D qIn, qOut;
        qIn.v[0] = { 1, 2, 3, 0xFFFFFFFF, 0, 0 };
        qIn.v[1] = { 2, 2, 3, 0xFFFFFFFF, 1, 0 };
        qIn.v[2] = { 1, 1, 3, 0xFFFFFFFF, 0, 1 };
        qIn.v[3] = { 2, 1, 3, 0xFFFFFFFF, 1, 1 };
        TransformQuad(qIn, M, qOut);
        Expect(qOut.v[0].x == 11.0f && qOut.v[0].y == 22.0f && qOut.v[0].z == 33.0f, "quad transform exact");
        Expect(qOut.v[1].u == 1.0f && qOut.v[2].v == 1.0f, "UV coordinates preserved across transform");
    }

    std::printf("\nTotal Failures: %d\n", g_fails);
    return g_fails ? 1 : 0;
}
