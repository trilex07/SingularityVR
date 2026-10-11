#include "../menu_hud_scale.h"
#include <cmath>
#include <cstdio>

static int g_fails = 0;

static void Expect(bool cond, const char* name) {
    if (cond) {
        std::printf("PASS %s\n", name);
        return;
    }
    std::printf("FAIL %s\n", name);
    ++g_fails;
}

static bool Near(float a, float b) {
    return std::fabs(a - b) < 0.0001f;
}

int main() {
    // Captured INI is 100. The old shared clamp produced 0.84 for class 1.
    Expect(Near(ClassBackbufferScale(1, true, 100), 0.70f),
           "class 1 with MenuInsetPct 100 uses the 0.70 safe area");
    Expect(Near(ClassBackbufferScale(1, true, 70), 0.70f),
           "class 1 at the source default stays 0.70");
    Expect(Near(ClassBackbufferScale(1, true, 50), 0.50f),
           "class 1 keeps an INI that is already tighter than 70");
    Expect(Near(ClassBackbufferScale(1, false, 100), 0.70f),
           "class 1 cap does not depend on the menuSafe flag");

    Expect(Near(ClassBackbufferScale(2, true, 100), 0.84f),
           "class 2 keeps the 0.84 limit");
    Expect(Near(ClassBackbufferScale(3, true, 100), 0.84f),
           "class 3 keeps the 0.84 limit");
    Expect(Near(ClassBackbufferScale(4, true, 100), 0.84f),
           "class 4 menuSafe clamp is unchanged");
    Expect(Near(ClassBackbufferScale(2, true, 60), 0.60f),
           "class 2 keeps a tighter INI");
    Expect(Near(ClassBackbufferScale(0, false, 100), 1.00f),
           "a non-menu backbuffer draw is not clamped");
    Expect(Near(ClassBackbufferScale(2, false, 100), 1.00f),
           "class 2 without menuSafe is not given the class-1 cap");
    return g_fails == 0 ? 0 : 1;
}
