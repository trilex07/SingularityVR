#pragma once

#include "async_log.h"

enum class IndependentReject : int {
    None = 0,
    Map = 1,
    PaletteRead = 2,
    DeltaPair = 3,
    Upload = 4,
};

inline void LogSupportGripTransition(bool acquired, long gunVerts, IndependentReject reject,
                                     bool weaponDeltaUsed, bool fallbackUsed) {
    Log("support-grip transition: %s gunVerts=%ld reject=%d weaponDelta=%d fallback=%d",
        acquired ? "ACQUIRED" : "RELEASED", gunVerts, static_cast<int>(reject),
        weaponDeltaUsed ? 1 : 0, fallbackUsed ? 1 : 0);
}
