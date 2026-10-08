#include "../input_remap.h"
#include <cassert>
#include <cstdio>

int main() {
    // Circle emits the game's native health action; stick-down emits native B/crouch.
    assert(vrinput::kCircleHealthMask == 0x0001);
    assert(vrinput::kStickDownCrouchMask == 0x2000);

    // Bandage animation is edge-triggered: holding Circle must not retrigger it every frame.
    vrinput::RisingEdge health;
    assert(health.Update(false) == false);
    assert(health.Update(true) == true);
    assert(health.Update(true) == false);
    assert(health.Update(true) == false);
    assert(health.Update(false) == false);
    assert(health.Update(true) == true);

    std::puts("input remap: Circle->Health, right-stick-down->Crouch, health rising-edge semantics");
    return 0;
}
