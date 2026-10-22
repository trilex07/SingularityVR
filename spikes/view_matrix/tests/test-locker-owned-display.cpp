#include "../locker_owned_display_planner.h"
#include "../locker_owned_display_session.h"
#include <cassert>
#include <cstdio>

int main() {
    using namespace lockerowned;

    assert(SameLockerFocusSession(true, 0x100u, 0x200u, 0x100u, 0x200u));
    assert(!SameLockerFocusSession(true, 0x100u, 0x200u, 0x101u, 0x200u));
    assert(!SameLockerFocusSession(false, 0x100u, 0x200u, 0x100u, 0x200u));
    assert(LockerFocusTransition(false, 0, 0, 0x100u, 0x200u));
    assert(!LockerFocusTransition(true, 0x100u, 0x200u, 0x100u, 0x200u));

    auto extras = PlanExtraTiles(0x3, 5799, 11387);
    assert(extras.count == 0);

    extras = PlanExtraTiles(0x7, 5799, 8297);
    assert(extras.count == 1);
    assert(extras.meshes[0] == 11387);

    extras = PlanExtraTiles(0x7, 5799, 8297);
    assert(extras.count == 1 && extras.meshes[0] == 11387);

    extras = PlanExtraTiles(0xF, 5799, 8297);
    assert(extras.count == 2);
    assert(extras.meshes[0] == 11387);
    assert(extras.meshes[1] == 11389);

    extras = PlanExtraTiles(0xF, 5799, 11387);
    assert(extras.count == 2);
    assert(extras.meshes[0] == 8297);
    assert(extras.meshes[1] == 11389);

    extras = PlanExtraTiles(0xF, 8297, 11389);
    assert(extras.count == 2);
    assert(extras.meshes[0] == 5799);
    assert(extras.meshes[1] == 11387);

    for (int i = 0; i < extras.count; ++i)
        assert(MeshAllowedInPlan(extras.meshes[i], 0xF));

    assert(!MeshAllowedInPlan(9999, 0xF));
    assert(!MeshAllowedInPlan(5799, 0x2));

    extras = PlanExtraTiles(0x3, 0, 11387);
    assert(extras.count == 1 && extras.meshes[0] == 5799);

    extras = PlanExtraTiles(0x3, 0, 0);
    assert(extras.count == 2);

    puts("test-locker-owned-display: PASS");
    return 0;
}
