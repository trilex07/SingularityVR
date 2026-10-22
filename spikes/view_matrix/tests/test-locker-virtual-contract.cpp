#include "../locker_virtual_contract.h"
#include <cassert>
#include <cstdio>

int main() {
    assert(lockervirtual::OrdinaryMesh(11387));
    assert(!lockervirtual::OrdinaryMesh(9999));

    int meshes[4]{};
    assert(lockervirtual::MeshesFromOwnedMask(0x3, meshes, 4) == 2);
    assert(meshes[0] == 5799 && meshes[1] == 11387);
    assert(lockervirtual::MeshesFromOwnedMask(0x5, meshes, 4) == 2);
    assert(meshes[0] == 5799 && meshes[1] == 8297);

    const auto plan = lockervirtual::PlanVirtualLoad(0xF);
    assert(plan.uiOrdinaryCount == 4);
    assert(plan.controllerOrdinaryCount == 2);

    const auto plan2 = lockervirtual::PlanVirtualLoad(0x3);
    assert(plan2.uiOrdinaryCount == 2);
    assert(plan2.controllerOrdinaryCount == 2);

    auto w = lockervirtual::DecidePopMenuControllerWrite(0, 4);
    assert(w.allow && w.controllerSlot == 0);
    w = lockervirtual::DecidePopMenuControllerWrite(1, 4);
    assert(w.allow && w.controllerSlot == 1);
    w = lockervirtual::DecidePopMenuControllerWrite(2, 4);
    assert(!w.allow);
    w = lockervirtual::DecidePopMenuControllerWrite(3, 2);
    assert(!w.allow);

    auto e = lockervirtual::DecideEquipIndex(1, 3);
    assert(e.allow && e.equippedIndex == 1);
    e = lockervirtual::DecideEquipIndex(3, 3);
    assert(!e.allow);
    e = lockervirtual::DecideEquipIndex(0, 0);
    assert(!e.allow);

    puts("test-locker-virtual-contract: PASS");
    return 0;
}
