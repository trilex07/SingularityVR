#pragma once
// Input-thread cache for WeaponLocker owned overlay (no dependency on diagnostic trace hooks).
#include "inventory_retention_policy.h"
#include "locker_owned_display_planner.h"
#include "locker_owned_display_session.h"
#include "native_owned_inventory.h"
#include "pause_rtt_probe.h"

extern volatile LONG g_lockerOwnedDisplay;
extern volatile LONG g_keepAcquiredWeapons;
extern volatile LONG g_vrUiWeaponLocker;
extern volatile LONG g_lockerDisplayNativeMesh[2];

void Log(_Printf_format_string_ const char* fmt, ...);

namespace LockerOwnedDisplayCache {

inline bool Enabled() {
    return InterlockedCompareExchange(&g_lockerOwnedDisplay, 0, 0) != 0 &&
           InterlockedCompareExchange(&g_keepAcquiredWeapons, 0, 0) != 0;
}

inline void PublishNativeMesh(int slot, int mesh) {
    if (slot < 0 || slot > 1) return;
    InterlockedExchange(&g_lockerDisplayNativeMesh[slot], mesh);
}

inline int ReadEquippedFNameMesh(uintptr_t movie, int propOff, int index) {
    if (propOff < 0 || !movie || index < 0 || index > 1) return 0;
    const uintptr_t arr = movie + (uintptr_t)propOff;
    if (!PauseRttProbe::SafeRange(arr, 12)) return 0;
    const int count = *reinterpret_cast<const int*>(arr + 4);
    if (index >= count) return 0;
    const uintptr_t data = PauseRttProbe::Pointer(arr);
    if (!data || !PauseRttProbe::SafeRange(data + (uintptr_t)index * 8, 8)) return 0;
    const NativeOwnedInventory::Name nm =
        *reinterpret_cast<const NativeOwnedInventory::Name*>(data + (uintptr_t)index * 8);
    char buf[96]{};
    NameFromIndex(nm.index, buf, sizeof(buf));
    if (!buf[0] || !strcmp(buf, "None")) return 0;
    return inventoryretention::WeaponMesh(buf);
}

inline uintptr_t g_movie = 0;
inline uintptr_t g_cls = 0;
inline int g_offEquipped = -1;

inline void RefreshNativeMeshesFromCachedOffset(uintptr_t movie) {
    if (!movie || g_offEquipped < 0) {
        PublishNativeMesh(0, 0);
        PublishNativeMesh(1, 0);
        return;
    }
    PublishNativeMesh(0, ReadEquippedFNameMesh(movie, g_offEquipped, 0));
    PublishNativeMesh(1, ReadEquippedFNameMesh(movie, g_offEquipped, 1));
}

inline bool g_sessionLive = false;
inline ULONGLONG g_nextRefresh = 0;

inline void OnFocus(uintptr_t movie, uintptr_t cls) {
    InterlockedExchange(&g_vrUiWeaponLocker, 1);
    if (lockerowned::SameLockerFocusSession(g_sessionLive, g_movie, g_cls, movie, cls)) return;

    g_movie = movie;
    g_cls = cls;
    g_sessionLive = true;
    g_offEquipped = cls ? PauseRttProbe::Property(cls, "mEquippedWeapons", "ArrayProperty") : -1;
    if (g_offEquipped < 0)
        Log("locker owned display: mEquippedWeapons offset unresolved on focus transition");

    if (!Enabled()) return;

    NativeOwnedInventory::Refresh();
    RefreshNativeMeshesFromCachedOffset(movie);
    const ULONGLONG now = GetTickCount64();
    g_nextRefresh = now + 250;

    static unsigned notes = 0;
    if (notes++ < 8)
        Log("locker owned display: focus transition native=%d,%d mask=0x%X off=%d",
            (int)InterlockedCompareExchange(&g_lockerDisplayNativeMesh[0], 0, 0),
            (int)InterlockedCompareExchange(&g_lockerDisplayNativeMesh[1], 0, 0),
            NativeOwnedInventory::Mask(), g_offEquipped);
}

inline void OnDefocus() {
    InterlockedExchange(&g_vrUiWeaponLocker, 0);
    g_sessionLive = false;
    g_movie = 0;
    g_cls = 0;
    g_offEquipped = -1;
    g_nextRefresh = 0;
    PublishNativeMesh(0, 0);
    PublishNativeMesh(1, 0);
}

inline void TickWhileActive() {
    if (!Enabled() || !InterlockedCompareExchange(&g_vrUiWeaponLocker, 0, 0)) return;
    if (!g_movie || g_offEquipped < 0) return;
    const ULONGLONG now = GetTickCount64();
    if (g_nextRefresh && now < g_nextRefresh) return;
    g_nextRefresh = now + 250;
    RefreshNativeMeshesFromCachedOffset(g_movie);
}

} // namespace LockerOwnedDisplayCache
