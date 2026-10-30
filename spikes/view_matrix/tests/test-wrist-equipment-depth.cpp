#include "../wrist_equipment_depth_policy.h"
#include <cassert>
#include <array>
#include <cstdio>
int main(){
    // Deterministic two-eye depth fixture. A wall (0.3) must hide a panel
    // (0.5); after the game's foreground clear, far depth (1) would expose it.
    std::array<float,2> depth{.3f,.3f};
    for(float z:depth)assert(!(.5f<=z));
    depth.fill(1.f);
    for(float z:depth)assert(.5f<=z);
    const wristhud::DepthEvidence atPresent{true,true,false,false};
    assert(!wristhud::ProductionDepth(atPresent));
    assert(!wristhud::MayDraw(true,false,atPresent));
    assert(wristhud::MayDraw(true,true,atPresent));
    assert(!wristhud::MaySuppress(true,true,true,atPresent));
    assert(!wristhud::MaySuppress(true,true,false,atPresent));
    const wristhud::DepthEvidence proven{true,true,true,true};
    assert(wristhud::MaySuppress(true,true,false,proven));
    assert(!wristhud::MaySuppress(false,true,false,proven));
    std::puts("PASS depth clear invalidates wall occlusion in both eyes; preview cannot suppress");
}
