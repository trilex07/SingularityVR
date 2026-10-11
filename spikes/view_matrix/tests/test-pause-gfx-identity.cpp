#include <cassert>
#include <cstdio>
#include "../pause_gfx_identity.h"

int main() {
    using namespace pausegfx;
    ResetLearnedFlashIdentity();
    constexpr LONG psA = 0x34275420;
    constexpr LONG vsA = 0x11111111;
    constexpr LONG declA = 0x2F515260;
    constexpr LONG psB = 0x34275F60;
    constexpr LONG vsB = 0x22222222;
    constexpr LONG declB = 0x2F514A00;

    assert(!FlashIdentityMatchSlot(0, psA, vsA, declA, false));
    assert(FlashIdentityMatchSlot(0, psA, vsA, declA, true));
    assert(FlashIdentityMatchSlot(0, psA, vsA, declA, false));
    assert(!FlashIdentityMatchSlot(0, psB, vsB, declB, false));

    assert(FlashIdentityMatchSlot(1, psB, vsB, declB, true));
    assert(!FlashIdentityMatchSlot(1, psA, vsA, declA, false));

    ResetLearnedFlashIdentity();
    assert(!FlashIdentityMatchSlot(0, psA, vsA, declA, false));
    assert(!FlashLearnAllowed(true, true, true, false));
    assert(!FlashLearnAllowed(true, false, true, true));
    assert(!FlashLearnAllowed(true, true, false, true));
    assert(!FlashLearnAllowed(false, true, true, true));
    assert(FlashLearnAllowed(true, true, true, true));

    std::puts("pause gfx identity: learn gated, ps+vs+decl triplet per stride");
    return 0;
}
