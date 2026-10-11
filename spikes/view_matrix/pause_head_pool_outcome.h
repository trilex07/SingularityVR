#pragma once
#include <windows.h>

enum class PauseHeadPoolOutcome {
    SceneFallback = 0,  // pool not used; caller runs the original draw once
    PooledOk,
    PooledDrawFailed,    // pooled draw ran and failed; do not draw again
    StateRestoreFailed,  // a state change or restore failed; do not draw again
};

struct PauseHeadPoolResult {
    PauseHeadPoolOutcome outcome = PauseHeadPoolOutcome::SceneFallback;
    HRESULT drawHr = S_OK;
    HRESULT stateHr = S_OK;
};

inline bool PoolResultAllowsSceneDraw(const PauseHeadPoolResult& r) {
    return r.outcome == PauseHeadPoolOutcome::SceneFallback && SUCCEEDED(r.stateHr);
}

inline bool PoolResultIsConsumedSuccess(const PauseHeadPoolResult& r) {
    return r.outcome == PauseHeadPoolOutcome::PooledOk && SUCCEEDED(r.drawHr) && SUCCEEDED(r.stateHr);
}

// Draw HRESULT wins when a pooled draw ran. A later scene success must not replace it.
inline HRESULT PoolResultReportedHr(const PauseHeadPoolResult& r) {
    if (r.outcome == PauseHeadPoolOutcome::PooledDrawFailed && FAILED(r.drawHr)) return r.drawHr;
    if (r.outcome == PauseHeadPoolOutcome::StateRestoreFailed) {
        if (FAILED(r.drawHr)) return r.drawHr;
        if (FAILED(r.stateHr)) return r.stateHr;
    }
    if (FAILED(r.stateHr)) return r.stateHr;
    return r.drawHr;
}
