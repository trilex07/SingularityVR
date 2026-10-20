#include "../weapon_wheel_layout.h"
#include "../weapon_wheel.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>

static int g_fails = 0;
static void Expect(bool cond, const char* name) {
    if (cond) { std::printf("PASS %s\n", name); return; }
    std::printf("FAIL %s\n", name);
    ++g_fails;
}

int main() {
    using namespace wheel_layout;
    const float pi = 3.14159265358979323846f;
    for(int extent : {1654,2160,3244,3308}) {
        const int r=RoundMetrics(3308).outerR;
        for(int pos=-100;pos<=200;++pos) {
            const float c=VisibleCenter((float)pos/100,extent,r)*extent;
            Expect(c-r>=7 && c+r<=extent-7,"complete wheel stays inside eye");
        }
    }

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
        Expect(m.outerR == 172 && m.innerR > 0 && m.innerR < m.outerR, "1080p radii");
        Expect(m.iconR < m.labelR && m.labelR < m.outerR, "anchor ordering");
        Metrics tiny = RoundMetrics(0);
        Expect(tiny.outerR == 80 && tiny.iconHalf >= 12, "degenerate height floors");
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

    // Slot labels come from the catalog gun order; every position is a real
    // weapon, so no slot needs a placeholder override. Unowned labelled
    // sectors (verts never held, not currently held) yield no label at all:
    // the wheel never names a weapon the player does not own yet.
    {
        const char* labels[4] = { "Centurion", "AR9 Valkyrie", "Spikeshot", "Kasimov SNV-E99" };
        const long verts[4] = { 5799, 11387, 8297, 11389 };
        const long seenNone[1] = { 0 };
        const long seenAr[2] = { 5799, 11387 };
        const long seenAll[4] = { 5799, 11387, 8297, 11389 };
        char dst[32];
        Expect(LabelForSlot(labels, verts, 4, 2, 5799, seenAll, 4, dst, sizeof(dst)) &&
               std::strcmp(dst, "SPIKESHOT") == 0, "plain slot label");
        Expect(LabelForSlot(labels, verts, 4, 0, 5799, seenNone, 1, dst, sizeof(dst)) &&
               std::strcmp(dst, "CENTURION") == 0, "held weapon always named");
        Expect(!LabelForSlot(labels, verts, 4, 2, 5799, seenNone, 1, dst, sizeof(dst)),
               "unowned sector unnamed");
        Expect(LabelForSlot(labels, verts, 4, 3, 5799, seenAll, 4, dst, sizeof(dst)) &&
               std::strcmp(dst, "KASIMOV SNV-E99") == 0, "seen weapon named");
        Expect(!LabelForSlot(labels, verts, 4, 3, 5799, nullptr, 0, dst, sizeof(dst)),
               "null seen set gates label");
        const char* empty[2] = { "", "" };
        const long ev[2] = { 0, 0 };
        Expect(!LabelForSlot(empty, ev, 2, 1, 0, seenNone, 1, dst, sizeof(dst)), "empty label refused");
        Expect(!LabelForSlot(labels, verts, 4, 9, 0, seenNone, 1, dst, sizeof(dst)), "oob slot refused");
        Expect(!LabelForSlot(nullptr, verts, 4, 1, 0, seenNone, 1, dst, sizeof(dst)), "null labels refused");
        // Long names truncate at kMaxLabel display glyphs, never overflow.
        const char* longL[1] = { "Very Long Weapon Name X" };
        const long lv[1] = { 0 };
        Expect(LabelForSlot(longL, lv, 1, 0, 0, seenNone, 1, dst, sizeof(dst)) &&
               std::strlen(dst) == (size_t)kMaxLabel, "long label truncates");
    }

    // Wheel positions map to catalog gun sectors in catalog order.
    {
        const long verts[8] = { 0, 5799, 11387, 8297, 11389, 0, 0, 0 };
        int order[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
        Expect(GunOrder(verts, 8, order, 8) == 4, "four gun positions");
        Expect(order[0] == 1 && order[1] == 2 && order[2] == 3 && order[3] == 4,
               "gun order follows catalog");
        Expect(GunOrder(verts, 8, order, 2) == 2, "order respects cap");
        const long bare[2] = { 0, 0 };
        Expect(GunOrder(bare, 2, order, 8) == 0, "no guns, no positions");
        Expect(GunOrder(nullptr, 8, order, 8) == 0, "null catalog refused");
        Expect(GunOrder(verts, 8, nullptr, 8) == 0, "null output refused");
    }

    // Hand-driven sector: synthetic eye looking -Z, 90-degree half tangents.
    {
        EyeBasis eye;
        eye.pos = { 0, 0, 0 };
        eye.fwd = { 0, 0, -1 };
        eye.right = { 1, 0, 0 };
        eye.up = { 0, 1, 0 };
        eye.tanHalfX = 1.0f;
        eye.tanHalfY = 1.0f;
        Expect(HandSector(0.0f, 0.0f, 0.0f, eye, 4) == -1, "hand still keeps highlight");
        Expect(HandSector(0.03f, 0.0f, 0.0f, eye, 4) == -1, "hand inside 6cm deadzone");
        Expect(HandSector(0.5f, 0.0f, 0.0f, eye, 4) == 1, "hand right selects right");
        Expect(HandSector(-0.5f, 0.0f, 0.0f, eye, 4) == 3, "hand left selects left");
        Expect(HandSector(0.0f, 0.5f, 0.0f, eye, 4) == 0, "hand up selects up");
        Expect(HandSector(0.0f, -0.5f, 0.0f, eye, 4) == 2, "hand down selects down");
        Expect(HandSector(0.0f, 0.0f, 0.5f, eye, 4) == -1, "hand pull-back has no direction");
        Expect(HandSector(0.0f, 0.0f, 3.0f, eye, 4) == -2, "hand behind eye refused");
        Expect(HandSector(0.0f, 0.0f, -0.5f, eye, 4) == -1, "hand forward push has no direction");
        EyeBasis bad = eye;
        bad.tanHalfX = 0.0f;
        Expect(HandSector(0.5f, 0.0f, 0.0f, bad, 4) == -2, "bad basis falls back to stick");
        Expect(HandSector(0.5f, 0.15f, 0.0f, eye, 6) == 1, "hand direction holds for 6 slots");
    }

    // FSM: handSector -2 keeps legacy stick; >= -1 drives the highlight.
    {
        using namespace weaponwheel;
        State s{};
        Expect(Update(s, true, false, true, true, false, 0, -1, 4, -2) == Event::Open, "hand-capable open");
        Expect(Update(s, true, false, true, true, false, 0, -1, 4, 1) == Event::SelectionChanged,
               "hand sector steers");
        Expect(s.selection == 1, "hand selection mirrored");
        Expect(Update(s, true, false, true, true, false, 0, -1, 4, -1) == Event::None,
               "hand deadzone keeps highlight");
        Expect(Update(s, true, false, true, true, false, 0, -1, 4, 1) == Event::None,
               "same hand sector is silent");
        Expect(Update(s, false, false, true, true, false, 0, 0, 4, 1) == Event::Commit,
               "hand release commits");
        State t{};
        Expect(Update(t, true, false, true, true, false, 1, 0, 4, -2) == Event::Open, "stick open");
        Expect(Update(t, true, false, true, true, false, 1, 0, 4, -2) == Event::SelectionChanged,
               "stick still works with -2");
    }

    // Touched rule: an untouched release cancels (tap safety); any touched
    // release commits, including back on sector 0 (every sector is a gun).
    {
        using namespace weaponwheel;
        State u{};
        Expect(Update(u, true, false, true, true, false, 0, -1, 4, -2) == Event::Open, "tap opens");
        Expect(Update(u, false, false, true, true, false, 0, 0, 4, -2) == Event::Cancel,
               "untouched release cancels");
        Expect(!u.open, "cancelled wheel closed");
        State v{};
        Expect(Update(v, true, false, true, true, false, 0, -1, 4, -2) == Event::Open, "touch opens");
        Expect(Update(v, true, false, true, true, false, 1, 0, 4, -2) == Event::SelectionChanged,
               "touch steers to 1");
        Expect(Update(v, true, false, true, true, false, 0, -1, 4, -2) == Event::SelectionChanged,
               "touch steers back to 0");
        Expect(v.selection == 0 && v.touched, "sector 0 touched");
        Expect(Update(v, false, false, true, true, false, 0, 0, 4, -2) == Event::Commit,
               "touched release on 0 commits");
    }

    if (g_fails == 0) std::puts("weapon wheel: round layout geometry + labels L1");
    return g_fails == 0 ? 0 : 1;
}
