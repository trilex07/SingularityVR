#pragma once
#include <windows.h>

// Bounded class-3 dispatch counters (per-second window reset from Present perf block).
struct PauseGfxDispatchCounters {
    volatile LONG hookDrawPrim = 0;
    volatile LONG hookDrawIndexedPrim = 0;
    volatile LONG hookDrawPrimUP = 0;
    volatile LONG hookDrawIndexedPrimUP = 0;
    volatile LONG hudStereoEnter = 0;
    volatile LONG redirectEligible = 0;
    volatile LONG redirectPooledOk = 0;
    volatile LONG redirectPooledFail = 0;
    volatile LONG redirectPoolFallback = 0;
    volatile LONG flashSignatureSeen = 0;
    volatile LONG flashSceneCommitted = 0;
    volatile LONG providerPauseProvided = 0;
};

extern PauseGfxDispatchCounters g_pauseGfxDispatch;

inline void PauseGfxDispatchResetWindow() {
    PauseGfxDispatchCounters z{};
    g_pauseGfxDispatch = z;
}
