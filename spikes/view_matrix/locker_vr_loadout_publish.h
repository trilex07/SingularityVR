#pragma once
// Scalar masks published from engine/input thread only (render/XR read these).
#include <cstdint>

extern volatile LONG g_publishedAvailableMask;
extern volatile LONG g_publishedOwnedMask;
extern volatile LONG g_publishedPendingMask;
extern volatile LONG g_publishedCanInteract;
extern volatile LONG g_publishedUiQueuedMask;

namespace LockerVrLoadoutPublish {

inline uint32_t Available() {
    return static_cast<uint32_t>(InterlockedCompareExchange(&g_publishedAvailableMask, 0, 0));
}

inline uint32_t Owned() {
    return static_cast<uint32_t>(InterlockedCompareExchange(&g_publishedOwnedMask, 0, 0));
}

inline uint32_t Pending() {
    return static_cast<uint32_t>(InterlockedCompareExchange(&g_publishedPendingMask, 0, 0));
}

inline bool CanInteract() {
    return InterlockedCompareExchange(&g_publishedCanInteract, 0, 0) != 0;
}

inline uint32_t UiQueued() {
    return static_cast<uint32_t>(InterlockedCompareExchange(&g_publishedUiQueuedMask, 0, 0));
}

inline void Clear() {
    InterlockedExchange(&g_publishedAvailableMask, 0);
    InterlockedExchange(&g_publishedOwnedMask, 0);
    InterlockedExchange(&g_publishedPendingMask, 0);
    InterlockedExchange(&g_publishedCanInteract, 0);
    InterlockedExchange(&g_publishedUiQueuedMask, 0);
}

inline void Publish(uint32_t available, uint32_t owned, uint32_t pending, bool canInteract, uint32_t uiQueued) {
    InterlockedExchange(&g_publishedAvailableMask, static_cast<LONG>(available));
    InterlockedExchange(&g_publishedOwnedMask, static_cast<LONG>(owned));
    InterlockedExchange(&g_publishedPendingMask, static_cast<LONG>(pending));
    InterlockedExchange(&g_publishedCanInteract, canInteract ? 1 : 0);
    InterlockedExchange(&g_publishedUiQueuedMask, static_cast<LONG>(uiQueued));
}

} // namespace LockerVrLoadoutPublish
