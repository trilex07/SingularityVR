#include "../health_gesture.h"
#include <cmath>
#include <cstdio>

static int g_fails = 0;
static void Expect(bool cond, const char* name) {
    if (cond) { std::printf("PASS %s\n", name); return; }
    std::printf("FAIL %s\n", name);
    ++g_fails;
}

int main() {
    using namespace healthbar;
    Expect(SegmentSlots() == 11, "eleven slots per mockup");
    // Head at origin looking -Z. Hand 30 cm ahead, slightly below eyes.
    Expect(FaceRaiseShow(0.0f, -0.05f, -0.30f, 0, 0, 0, 0, 0, -1), "raised hand shows");
    Expect(!FaceRaiseShow(0.0f, -0.05f, -0.55f, 0, 0, 0, 0, 0, -1), "extended support hand stays hidden");
    Expect(!FaceRaiseShow(0.0f, -0.05f, -0.90f, 0, 0, 0, 0, 0, -1), "far hand stays hidden");
    Expect(!FaceRaiseShow(0.0f, -0.05f, 0.30f, 0, 0, 0, 0, 0, -1), "hand behind face stays hidden");
    Expect(!FaceRaiseShow(0.0f, -0.60f, -0.20f, 0, 0, 0, 0, 0, -1), "hand at waist stays hidden");
    Expect(!FaceRaiseShow(0.0f, -0.05f, -0.30f, 0, 0, 0, 0, 0, 1), "facing away stays hidden");
    // Hysteresis keeps the 50-60 cm band, but lowering always hides.
    Expect(!FaceRaiseHide(0.0f, -0.05f, -0.55f, 0, 0, 0, 0, 0, -1), "band keeps bar");
    Expect(FaceRaiseHide(0.0f, -0.35f, -0.20f, 0, 0, 0, 0, 0, -1), "lowering near body hides");
    Expect(!FaceRaiseShow(0.0f, -0.25f, -0.20f, 0, 0, 0, 0, 0, -1), "hand below face stays hidden");
    Expect(FaceRaiseHide(0.0f, -0.05f, -0.90f, 0, 0, 0, 0, 0, -1), "outer radius hides");
    Expect(FaceRaiseHide(0.0f, -0.05f, 0.30f, 0, 0, 0, 0, 0, -1), "behind face hides");
    // Segment mapping.
    // Native floats travel as int32 reads: reinterpret before scaling.
    Expect(HpFromBits((std::int32_t)0x43100000) == 144.0f, "hp bits to float");
    Expect(FilledSegments(HpFromBits((std::int32_t)0x43100000),
                          HpFromBits((std::int32_t)0x43100000)) == 11, "full hp bits full bar");
    Expect(FilledSegments(HpFromBits((std::int32_t)0x42900000),
                          HpFromBits((std::int32_t)0x43100000)) == 6, "half hp bits half bar");
    Expect(HpFromBits(1125515264) > 0.0f, "session bits positive float");
    {
        const auto full = DecodeLiveHealth((std::int32_t)0x43100000, (std::int32_t)0x43100000);
        const auto half = DecodeLiveHealth((std::int32_t)0x42900000, (std::int32_t)0x43100000);
        const auto empty = DecodeLiveHealth(0, (std::int32_t)0x43100000);
        Expect(full.valid && full.fraction == 1.0f && full.cells10 == 10 &&
               full.plateSlots == 11, "live full: wrist and plate share float source");
        Expect(half.valid && half.fraction == 0.5f && half.cells10 == 5 &&
               half.plateSlots == 6, "live half: no integer-bit ratio");
        Expect(empty.valid && empty.fraction == 0.0f && empty.cells10 == 0,
               "live empty: zero cells");
        Expect(!DecodeLiveHealth((std::int32_t)0x7f800000, (std::int32_t)0x43100000).valid,
               "infinite health hidden");
        Expect(!DecodeLiveHealth((std::int32_t)0x43100000, 0).valid,
               "unknown max hidden");
    }
    Expect(FilledSegments(0.0f, 100.0f) == 0, "empty bar");
    Expect(FilledSegments(50.0f, 100.0f) == 6, "half rounds to six");
    Expect(FilledSegments(150.0f, 100.0f) == 11, "overheal clamps full");
    Expect(FilledSegments(100.0f, 0.0f) == -1, "zero max unknown");
    Expect(FilledSegments(-5.0f, 100.0f) == -1, "negative hp unknown");
    Expect(FilledSegments(50.0f, 100.0f) >= 0, "normal reading known");
    {
        const float nan = std::nanf("");
        Expect(FilledSegments(nan, 100.0f) == -1, "NaN hp unknown");
        Expect(FilledSegments(50.0f, nan) == -1, "NaN max unknown");
    }
    // Slot fractions stay inside the plate and tile left to right.
    {
        float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
        SlotFrac(0, x0, y0, x1, y1);
        Expect(x0 > 0.15f && x0 < 0.17f && x1 > x0, "slot 0 fractions");
        float px1 = 0;
        SlotFrac(10, x0, y0, x1, y1);
        px1 = x1;
        Expect(px1 < 1.0f && y0 > 0.18f && y1 < 0.97f, "slot 10 inside plate");
        float ax0 = 0, ay0 = 0, ax1 = 0, ay1 = 0, bx0 = 0, by0 = 0, bx1 = 0, by1 = 0;
        SlotFrac(3, ax0, ay0, ax1, ay1);
        SlotFrac(4, bx0, by0, bx1, by1);
        Expect(bx0 + 0.001f >= ax1 && ay0 == by0 && ay1 == by1, "slots tile in order");
        SlotFrac(-5, ax0, ay0, ax1, ay1);
        SlotFrac(99, bx0, by0, bx1, by1);
        Expect(ax0 >= 0.0f && bx1 <= 1.0f, "slot indices clamp");
    }
    if (g_fails == 0) std::puts("health bar: face-raise gesture + segments L1");
    return g_fails == 0 ? 0 : 1;
}
