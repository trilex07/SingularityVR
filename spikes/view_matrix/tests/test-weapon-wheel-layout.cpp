#include "../weapon_wheel_layout.h"
#include "../weapon_wheel.h"
#include <cmath>
#include <cstdio>
#include <cstring>

static int g_fails = 0;
static void Expect(bool cond, const char* name) {
    if (cond) { std::printf("PASS %s\n", name); return; }
    std::printf("FAIL %s\n", name);
    ++g_fails;
}

int main() {
    using namespace wheel_layout;
    const float pi = 3.14159265358979323846f;

    // Sector centres: 0 = up, clockwise. Matches the square-card placement
    // (cos/sin at -pi/2 + 2*pi*i/slots) and SectorForStick.
    Expect(fabsf(SectorCenter(0, 4) - 0.0f) < 1e-6f, "slot0 centre zero");
    Expect(fabsf(SectorCenter(1, 4) - pi * 0.5f) < 1e-6f, "slot1 quarter turn");
    Expect(fabsf(SectorSpan(4) - pi * 0.5f) < 1e-6f, "span 4 slots");
    Expect(fabsf(SectorSpan(6) - pi / 3.0f) < 1e-6f, "span 6 slots");

    // Stick round-trip: a stick pushed toward a sector centre selects it.
    for (int slots = 2; slots <= 8; ++slots) {
        for (int i = 0; i < slots; ++i) {
            const float c = SectorCenter(i, slots);
            const float sx = sinf(c), sy = -cosf(c);
            char name[64];
            std::snprintf(name, sizeof(name), "stick round-trip slots=%d sector=%d", slots, i);
            Expect(weaponwheel::SectorForStick(sx, sy, slots) == i, name);
        }
    }

    // Wedge bounds shrink symmetrically around the centre and keep >=20% span.
    for (int slots = 2; slots <= 8; ++slots) {
        float a0 = 0, a1 = 0;
        WedgeBounds(1, slots, 0.0f, a0, a1);
        char name[64];
        std::snprintf(name, sizeof(name), "contiguous slots=%d", slots);
        Expect(a0 < a1 && fabsf((a0 + a1) * 0.5f - SectorCenter(1, slots)) < 1e-5f, name);
        WedgeBounds(1, slots, 10.0f, a0, a1); // absurd gap clamps, never inverts
        std::snprintf(name, sizeof(name), "gap clamps slots=%d", slots);
        Expect(a0 < a1, name);
    }

    // Screen mapping: sector 0 lands above the anchor (y decreases).
    {
        float x = 0, y = 0;
        WedgePoint(100.0f, 100.0f, 50.0f, SectorCenter(0, 4), x, y);
        Expect(x == 100.0f && y == 50.0f, "sector0 is up");
        WedgePoint(100.0f, 100.0f, 50.0f, SectorCenter(1, 4), x, y);
        Expect(x == 150.0f && y == 100.0f, "sector1 is right");
    }

    // Metrics scale with frame height and stay sane at degenerate input.
    {
        Metrics m = RoundMetrics(1080);
        Expect(m.outerR == 216 && m.innerR > 0 && m.innerR < m.outerR, "1080p radii");
        Expect(m.iconR < m.labelR && m.labelR < m.outerR, "anchor ordering");
        Metrics tiny = RoundMetrics(0);
        Expect(tiny.outerR == 96 && tiny.iconHalf >= 12, "degenerate height floors");
    }

    // Glyph sanitation is explicit: lowercase folds, supported symbols stay,
    // everything else (including '/' and UTF-8 diacritic bytes) is a space.
    {
        char dst[32];
        SanitizeLabel("Kasimov SNV-E99", dst, sizeof(dst));
        Expect(std::strcmp(dst, "KASIMOV SNV-E99") == 0, "label folds case");
        SanitizeLabel("AR9 Valkyrie", dst, sizeof(dst));
        Expect(std::strcmp(dst, "AR9 VALKYRIE") == 0, "mixed case folds");
        SanitizeLabel("12 / 72", dst, sizeof(dst));
        Expect(std::strcmp(dst, "12   72") == 0, "slash becomes space");
        SanitizeLabel("Seeker/Grenades", dst, sizeof(dst));
        Expect(std::strcmp(dst, "SEEKER GRENADES") == 0, "slash in mockup label");
        SanitizeLabel("TMD/Tool", dst, sizeof(dst));
        Expect(std::strcmp(dst, "TMD TOOL") == 0, "slash in tmd label");
        SanitizeLabel(nullptr, dst, sizeof(dst));
        Expect(dst[0] == 0, "null sanitises empty");
    }

    // Slot labels come from the catalog; slot 0 shows the held weapon.
    {
        const char* labels[4] = { "Obecna", "Centurion", "AR9 Valkyrie", "Spikeshot" };
        const long verts[4] = { 0, 5799, 11387, 8297 };
        char dst[32];
        Expect(LabelForSlot(labels, verts, 4, 2, 5799, dst, sizeof(dst)) &&
               std::strcmp(dst, "AR9 VALKYRIE") == 0, "plain slot label");
        Expect(LabelForSlot(labels, verts, 4, 0, 11387, dst, sizeof(dst)) &&
               std::strcmp(dst, "AR9 VALKYRIE") == 0, "slot0 shows held weapon");
        Expect(LabelForSlot(labels, verts, 4, 0, 0, dst, sizeof(dst)) &&
               std::strcmp(dst, "OBECNA") == 0, "slot0 fallback without verts");
        Expect(LabelForSlot(labels, verts, 4, 0, 12345, dst, sizeof(dst)) &&
               std::strcmp(dst, "OBECNA") == 0, "slot0 fallback unknown verts");
        const char* empty[2] = { "", "" };
        const long ev[2] = { 0, 0 };
        Expect(!LabelForSlot(empty, ev, 2, 1, 0, dst, sizeof(dst)), "empty label refused");
        Expect(!LabelForSlot(labels, verts, 4, 9, 0, dst, sizeof(dst)), "oob slot refused");
        Expect(!LabelForSlot(nullptr, verts, 4, 1, 0, dst, sizeof(dst)), "null labels refused");
        // Long names truncate at kMaxLabel display glyphs, never overflow.
        const char* longL[1] = { "Very Long Weapon Name X" };
        const long lv[1] = { 5799 };
        Expect(LabelForSlot(longL, lv, 1, 0, 0, dst, sizeof(dst)) &&
               std::strlen(dst) == (size_t)kMaxLabel, "long label truncates");
    }

    if (g_fails == 0) std::puts("weapon wheel: round layout geometry + labels L1");
    return g_fails == 0 ? 0 : 1;
}
