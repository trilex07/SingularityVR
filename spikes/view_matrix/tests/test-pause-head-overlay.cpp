#include <cassert>
#include <cstdio>
#include "../pause_menu_head_overlay.h"

static void ExpectGfxBatch(bool expect, int menuClass, bool menuLayout, bool nativeReady,
                           bool gamePaused, bool rtIsScene, bool rtIsBackbuffer) {
    const bool got = pausehead::GfxPauseMenuMonoBatch(
        menuClass, menuLayout, nativeReady, gamePaused, rtIsScene, rtIsBackbuffer, true, true, true,
        false);
    assert(got == expect);
}

int main() {
    using namespace pausehead;
    assert(!RedirectOrthoDraws(3, false, true, true, true, false));
    assert(RedirectOrthoDraws(3, true, true, true, true, false));
    ExpectGfxBatch(true, 3, true, true, true, true, false);
    ExpectGfxBatch(false, 3, false, true, true, true, false);
    ExpectGfxBatch(false, 3, true, false, true, true, false);
    ExpectGfxBatch(false, 3, true, true, false, true, false);
    ExpectGfxBatch(false, 3, true, true, true, false, false);
    ExpectGfxBatch(false, 3, true, true, true, true, true);
    ExpectGfxBatch(false, 2, true, true, true, true, false);
    assert(ShouldClearPool(0, 1));
    assert(!ShouldClearPool(1, 1));
    const RedirectPlan plan = PlanGfxRedirect(0, 42, true, true);
    assert(plan.attemptRedirect && plan.clearPool && plan.frameKey == 42);
    assert(!PlanGfxRedirect(0, 42, false, true).attemptRedirect);
    assert(!PlanGfxRedirect(0, 42, true, false).attemptRedirect);
    assert(MustRestoreAfterBindFailure(BindStep::SetRenderTargetFailed));
    assert(MustRestoreAfterBindFailure(BindStep::ClearFailed));
    assert(!MustRestoreAfterBindFailure(BindStep::PoolUnavailable));
    assert(CaptureAfterDrawAllowed(BindStep::Ok, true));
    assert(!CaptureAfterDrawAllowed(BindStep::Ok, false));
    assert(!CaptureAfterDrawAllowed(BindStep::SetViewportFailed, true));
    std::puts("pause head overlay: Gfx mono gating, pool clear key, bind/capture policy");
    return 0;
}
