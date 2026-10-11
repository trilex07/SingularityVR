#include <cassert>
#include <cstdio>
#include "../pause_vr_settings.h"

static void ExpectBand(int menuClass, float v0, float v1) {
    using namespace pausevrsettings;
    float got0 = 0.0f, got1 = 0.0f;
    assert(Row(menuClass, got0, got1));
    assert(got0 == v0 && got1 == v1);
    assert(Hit(menuClass, .50f, (v0 + v1) * .5f));
    assert(Hit(menuClass, kU0, v0));
    assert(Hit(menuClass, kU1, v1));
    assert(!Hit(menuClass, .249f, (v0 + v1) * .5f));
    assert(!Hit(menuClass, .751f, (v0 + v1) * .5f));
    assert(!Hit(menuClass, .50f, v0 - .001f));
    assert(!Hit(menuClass, .50f, v1 + .001f));
}

int main() {
    using namespace pausevrsettings;
    ExpectBand(2, kMainV0, kMainV1);
    ExpectBand(3, kPauseV0, kPauseV1);
    assert(kMainV0 == 0.70f && kMainV1 == 0.80f);
    assert(kPauseV0 == 0.60f && kPauseV1 == 0.70f);
    assert(Hit(2, .50f, .75f));
    assert(!Hit(2, .50f, .65f));
    assert(Hit(3, .50f, .65f));
    assert(!Hit(3, .50f, .75f));
    assert(!Hit(1, .50f, .65f));
    assert(!Hit(4, .50f, .75f));
    float unused0 = 1.0f, unused1 = 1.0f;
    assert(!Row(1, unused0, unused1));
    assert(unused0 == 1.0f && unused1 == 1.0f);
    std::puts("pause VR settings row: class 2 stays at V 0.70-0.80, class 3 uses V 0.60-0.70");
    return 0;
}
