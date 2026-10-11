#include <cassert>
#include <cstdio>
#include "../pause_menu_head_overlay.h"

static void ExpectGfxBatch(bool expect, int menuClass, bool menuLayout, bool nativeReady,
                           bool gamePaused, bool noteOpen, float clipX, float clipY) {
    const bool got = pausehead::GfxPauseMenuMonoBatch(
        menuClass, menuLayout, nativeReady, gamePaused, noteOpen, true, false, clipX, clipY, true,
        true, true, false);
    assert(got == expect);
}

int main() {
    using namespace pausehead;
    assert(!RedirectOrthoDraws(3, false, true, true, true, false));
    assert(RedirectOrthoDraws(3, true, true, true, true, false));
    ExpectGfxBatch(true, 3, true, true, true, false, 0.0f, 0.0f);
    ExpectGfxBatch(false, 3, true, true, true, false, 0.95f, 0.0f);
    ExpectGfxBatch(false, 3, true, true, true, false, 0.0f, -0.95f);
    ExpectGfxBatch(false, 3, true, true, true, true, 0.0f, 0.0f);
    ExpectGfxBatch(false, 3, false, true, true, false, 0.0f, 0.0f);
    ExpectGfxBatch(false, 2, true, true, true, false, 0.0f, 0.0f);
    assert(GfxOrthoClipLooksLikePauseMenu(0.5f, -0.5f));
    assert(!GfxOrthoClipLooksLikePauseMenu(kPauseGfxClipEdge + 0.01f, 0.0f));
    assert(ShouldClearPool(0, 1));
    const RedirectPlan plan = PlanGfxRedirect(0, 42, true, true);
    assert(plan.attemptRedirect && plan.clearPool && plan.frameKey == 42);
    assert(MustRestoreAfterBindFailure(BindStep::SetRenderTargetFailed));
    assert(CaptureAfterDrawAllowed(BindStep::Ok, true));
    assert(!CaptureAfterDrawAllowed(BindStep::ClearFailed, true));
    const DeviceStateFields restore = PlanDeviceRestore(BindStep::SetViewportFailed, true);
    assert(MustRestoreDeviceState(restore));
    assert(!MustRestoreDeviceState(PlanDeviceRestore(BindStep::PoolUnavailable, false)));
    std::puts("pause head overlay: Gfx clip gate, D3D restore policy, capture guards");
    return 0;
}
