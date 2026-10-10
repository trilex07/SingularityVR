#include "../prompt_placement.h"
#include <cassert>
#include <cstdio>

namespace {
bool close(float a, float b, float e = 1e-5f) {
    float d = a > b ? a - b : b - a;
    return d < e;
}

void safe_area_tests() {
    using namespace promptplace;
    SafeArea s;
    // Rendered wider than visible -> centered crop fraction.
    assert(SafeAreaFromFov(1.04f, 0.80f, s));
    assert(close(s.visibleFraction, 0.80f / 1.04f));
    assert(close(s.topY, (1.0f - s.visibleFraction) * 0.5f));
    assert(close(s.bottomY, 1.0f - s.topY));
    // Fully visible -> identity.
    assert(SafeAreaFromFov(0.80f, 0.90f, s));
    assert(close(s.visibleFraction, 1.0f) && close(s.topY, 0.0f) && close(s.bottomY, 1.0f));
    // Degenerate input rejected.
    assert(!SafeAreaFromFov(0.0f, 0.8f, s));
    assert(!SafeAreaFromFov(1.0f, -1.0f, s));
    std::puts("safe-area: crop fraction centering, identity, rejection");
}

void legend_row_tests() {
    using namespace promptplace;
    SafeArea full{1.0f, 0.0f, 1.0f};
    float y = 0;
    // Two-row legend: rows stack upward from the bottom margin, ordered.
    assert(LegendRowY(full, 0, 0.05f, 0.02f, y) && close(y, 1.0f - 0.02f - 0.025f));
    float y1 = 0;
    assert(LegendRowY(full, 1, 0.05f, 0.02f, y1) && y1 < y && close(y - y1, 0.05f));
    // Cropped frustum: rows stay inside the safe area (never in the cut zone).
    SafeArea crop;
    assert(SafeAreaFromFov(1.04f, 0.80f, crop));
    for (unsigned r = 0; r < 4; ++r) {
        assert(LegendRowY(crop, r, 0.05f, 0.02f, y));
        assert(y >= crop.topY && y <= crop.bottomY);
    }
    // Oversize row rejected rather than placed off-area.
    assert(!LegendRowY(full, 30, 0.05f, 0.02f, y));
    std::puts("legend: bottom-anchored stacking, safe-area containment");
}

void eye_base_tests() {
    using namespace promptplace;
    float x = 0;
    assert(EyeBaseX(2684.0f, 0, 0.12f, x) && close(x, 1342.0f * 0.12f));
    assert(EyeBaseX(2684.0f, 1, 0.12f, x) && close(x, 1342.0f + 1342.0f * 0.12f));
    assert(!EyeBaseX(1.0f, 0, 0.12f, x));
    assert(!EyeBaseX(2684.0f, 2, 0.12f, x));
    std::puts("eye-base: SBS duplication origins");
}
} // namespace

int main() {
    safe_area_tests();
    legend_row_tests();
    eye_base_tests();
    std::puts("PASS");
    return 0;
}
