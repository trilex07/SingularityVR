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
    if (g_fails == 0) std::puts("health bar: face-raise gesture + segments L1");
    return g_fails == 0 ? 0 : 1;
}
