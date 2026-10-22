#pragma once
#include <windows.h>

namespace LockerVrLoadoutPreviewPolicy {

inline bool BlocksAcquisition(bool previewOnly) { return previewOnly; }

inline void ClearQueuedMasks(volatile LONG* queued, volatile LONG* uiQueued,
                             volatile LONG* postPop, volatile LONG* requested,
                             volatile LONG* publishedUiQueued) {
    InterlockedExchange(queued, 0);
    InterlockedExchange(uiQueued, 0);
    InterlockedExchange(postPop, 0);
    InterlockedExchange(requested, 0);
    InterlockedExchange(publishedUiQueued, 0);
}

} // namespace LockerVrLoadoutPreviewPolicy
