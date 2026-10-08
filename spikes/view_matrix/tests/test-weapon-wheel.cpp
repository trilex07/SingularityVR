#include "../weapon_wheel.h"
#include <cassert>
#include <cstdio>

int main() {
    using namespace weaponwheel;
    assert(ClampSlots(0) == 2 && ClampSlots(4) == 4 && ClampSlots(99) == 8);
    assert(SectorForStick(0.0f, 0.0f, 4) == -1);
    assert(SectorForStick(0.0f, -1.0f, 4) == 0); // up
    assert(SectorForStick(1.0f, 0.0f, 4) == 1);  // right
    assert(SectorForStick(0.0f, 1.0f, 4) == 2);  // down
    assert(SectorForStick(-1.0f, 0.0f, 4) == 3); // left
    assert(SectorForStick(0.0f, -0.36f, 6) == 0);
    assert(SectorForStick(0.01f, 0.36f, 6) == 3);
    assert(SectorForStick(0.1f, 0.1f, 8) == -1);
    std::puts("weapon wheel: slot clamp, deadzone, up/right/down/left and six/eight-sector mapping");
}
