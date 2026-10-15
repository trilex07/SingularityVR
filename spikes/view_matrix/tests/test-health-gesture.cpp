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
    Expect(!FaceRaiseShow(0.0f, -0.05f, -0.60f, 0, 0, 0, 0, 0, -1), "far hand stays hidden");
    Expect(!FaceRaiseShow(0.0f, -0.05f, 0.30f, 0, 0, 0, 0, 0, -1), "hand behind face stays hidden");
    Expect(!FaceRaiseShow(0.0f, -0.60f, -0.20f, 0, 0, 0, 0, 0, -1), "hand at waist stays hidden");
    Expect(!FaceRaiseShow(0.0f, -0.05f, -0.30f, 0, 0, 0, 0, 0, 1), "facing away stays hidden");
    // Hysteresis: shown bar survives the 40-55 cm band, dies outside it.
    Expect(!FaceRaiseHide(0.0f, -0.05f, -0.45f, 0, 0, 0, 0, 0, -1), "band keeps bar");
    Expect(FaceRaiseHide(0.0f, -0.05f, -0.60f, 0, 0, 0, 0, 0, -1), "outer radius hides");
    Expect(FaceRaiseHide(0.0f, -0.05f, 0.30f, 0, 0, 0, 0, 0, -1), "behind face hides");
    // Segment mapping.
    Expect(FilledSegments(100.0f, 100.0f) == 11, "full bar");
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
