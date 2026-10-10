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

    // Deterministic R3 input script: rising edge opens once, holding does not
    // repeat it, right-stick movement changes the highlighted sector, and the
    // falling edge commits exactly once. This is the same state machine used
    // by the runtime path and by DesktopAutomation in Steam/XRSim smoke runs.
    State s{};
    assert(Update(s, false, false, true, true, false, 0, 0, 4) == Event::None);
    assert(Update(s, true,  false, true, true, false, 0, -1, 4) == Event::Open);
    assert(s.open && s.selection == 0);
    assert(Update(s, true, false, true, true, false, 1, 0, 4) == Event::SelectionChanged);
    assert(s.selection == 1);
    assert(Update(s, true, false, true, true, false, 1, 0, 4) == Event::None);
    assert(Update(s, false, false, true, true, false, 1, 0, 4) == Event::Commit);
    assert(!s.open && s.selection == 0);
    assert(Update(s, false, false, true, true, false, 0, 0, 4) == Event::None);

    // L3 cancels a held wheel and cannot leak a commit on R3 release.
    assert(Update(s, true,  false, true, true, false, 0, -1, 4) == Event::Open);
    assert(Update(s, true,  true,  true, true, false, 0, -1, 4) == Event::Cancel);
    assert(Update(s, false, false, true, true, false, 0, 0, 4) == Event::None);
    std::puts("weapon wheel: slot/deadzone mapping and deterministic R3 down-hold-stick-up/commit/cancel edges");
}
