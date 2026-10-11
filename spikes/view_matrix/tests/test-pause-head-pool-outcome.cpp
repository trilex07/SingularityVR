#include <cassert>
#include <cstdio>
#include "../pause_head_pool_outcome.h"

int main() {
    PauseHeadPoolResult failedDraw{PauseHeadPoolOutcome::PooledDrawFailed, E_FAIL, S_OK};
    assert(!PoolResultAllowsSceneDraw(failedDraw));
    assert(!PoolResultIsConsumedSuccess(failedDraw));
    assert(PoolResultReportedHr(failedDraw) == E_FAIL);
    const HRESULT sceneOk = S_OK;
    assert(PoolResultReportedHr(failedDraw) != sceneOk);

    PauseHeadPoolResult failedDrawAndRestore{PauseHeadPoolOutcome::StateRestoreFailed,
                                              (HRESULT)0x8876086C, E_FAIL};
    assert(!PoolResultAllowsSceneDraw(failedDrawAndRestore));
    assert(PoolResultReportedHr(failedDrawAndRestore) == (HRESULT)0x8876086C);

    PauseHeadPoolResult restoreOnly{PauseHeadPoolOutcome::StateRestoreFailed, S_OK, E_FAIL};
    assert(!PoolResultAllowsSceneDraw(restoreOnly));
    assert(PoolResultReportedHr(restoreOnly) == E_FAIL);

    PauseHeadPoolResult fallback{PauseHeadPoolOutcome::SceneFallback, S_OK, S_OK};
    assert(PoolResultAllowsSceneDraw(fallback));
    assert(!PoolResultIsConsumedSuccess(fallback));

    PauseHeadPoolResult ok{PauseHeadPoolOutcome::PooledOk, S_OK, S_OK};
    assert(PoolResultIsConsumedSuccess(ok));
    assert(!PoolResultAllowsSceneDraw(ok));

    PauseHeadPoolResult okDrawBadRestore{PauseHeadPoolOutcome::StateRestoreFailed, S_OK, E_FAIL};
    assert(!PoolResultIsConsumedSuccess(okDrawBadRestore));

    std::puts("pause head pool outcome: draw HRESULT kept, no masked scene success");
    return 0;
}
