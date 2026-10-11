#pragma once

enum class PauseHeadPoolOutcome {
    SceneFallback = 0,  // pool not used; caller must run original scene draw
    PooledOk,
    PooledDrawFailed,  // pool bound but orig draw failed; do not count as poolOk
};
