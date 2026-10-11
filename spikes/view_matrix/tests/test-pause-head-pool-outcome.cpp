#include <cassert>
#include <cstdio>
#include "../pause_head_pool_outcome.h"

int main() {
    assert((int)PauseHeadPoolOutcome::SceneFallback == 0);
    assert(PauseHeadPoolOutcome::PooledOk != PauseHeadPoolOutcome::SceneFallback);
    assert(PauseHeadPoolOutcome::PooledDrawFailed != PauseHeadPoolOutcome::PooledOk);
    std::puts("pause head pool outcome: explicit pooled / fallback / failed");
    return 0;
}
