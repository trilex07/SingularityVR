#include "../locker_vr_loadout_preview_policy.h"
#include <cassert>
#include <cstdio>

int main() {
    using namespace LockerVrLoadoutPreviewPolicy;
    volatile LONG queued = 0xE, uiQueued = 0xE, postPop = 0x6, requested = 1, published = 0xE;
    assert(!BlocksAcquisition(false));
    assert(queued == 0xE && uiQueued == 0xE && postPop == 0x6 && requested == 1 && published == 0xE);
    assert(BlocksAcquisition(true));
    ClearQueuedMasks(&queued, &uiQueued, &postPop, &requested, &published);
    assert(queued == 0 && uiQueued == 0 && postPop == 0 && requested == 0 && published == 0);

    int nativeFactoryCalls = 0;
    int nativePickupCalls = 0;
    const bool modes[] = {true, false};
    for (const bool previewOnly : modes) {
        if (!BlocksAcquisition(previewOnly)) {
            ++nativeFactoryCalls;
            ++nativePickupCalls;
        }
    }
    assert(nativeFactoryCalls == 1 && nativePickupCalls == 1);
    std::puts("locker preview policy ON blocks, OFF preserves acquisition: PASS");
    return 0;
}
