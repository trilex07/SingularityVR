#pragma once
#include "MinHook.h"
#include "locker_vr_loadout_acquire.h"
#include "locker_vr_loadout_publish.h"
#include "locker_vr_loadout_runtime.h"
#include "native_owned_inventory.h"
#include "pause_rtt_probe.h"
#include <cstring>
#include <intrin.h>

extern volatile LONG g_lockerVrLoadout;
extern volatile LONG g_keepAcquiredWeapons;
extern volatile LONG g_vrUiWeaponLocker;
extern volatile LONG g_weaponWheelOpen;
extern volatile LONG g_panelOpen;
extern volatile LONG g_inCinematic;
extern volatile LONG g_gamePaused;
extern uintptr_t g_textLo, g_textHi;
extern uintptr_t g_playerPawn;
void Log(_Printf_format_string_ const char* fmt, ...);

namespace LockerVrLoadout {

constexpr uintptr_t kWeaponLockerVtable = 0x019A0948u;
constexpr uintptr_t kSlotPopMenu = 0x190u;
constexpr uintptr_t kSlotLoad = 0x1F4u;
constexpr uintptr_t kFnPopMenu = 0x00F17F60u;
constexpr uintptr_t kFnLoad = 0x00F23840u;
constexpr uintptr_t kFnLoadUpgrades = 0x00F87C80u;
constexpr uintptr_t kFnEquip = 0x00F00F50u;

using FrameFn = void(__thiscall*)(void* self, void* frame);
using ScriptExecFn = void(__thiscall*)(void* self, void* frame, void* result);

inline volatile LONG g_pageSignal = 0;
inline volatile LONG g_hooksLive = 0;
inline volatile LONG g_queuedMask = 0;
inline volatile LONG g_uiQueuedMask = 0;
inline volatile LONG g_postPopAcquireMask = 0;
inline volatile LONG g_acquireRequested = 0;

inline FrameFn g_origPop = nullptr;
inline FrameFn g_origLoad = nullptr;
inline ScriptExecFn g_origLoadUpgrades = nullptr;

inline thread_local int t_reenter = 0;

inline bool Enabled() {
    return InterlockedCompareExchange(&g_lockerVrLoadout, 0, 0) != 0 &&
           InterlockedCompareExchange(&g_keepAcquiredWeapons, 0, 0) != 0;
}

inline bool IsEquipPage() {
    return InterlockedCompareExchange(&g_pageSignal, 0, 0) == 1;
}

inline bool EngineCanInteract() {
    if (!Enabled()) return false;
    if (!InterlockedCompareExchange(&g_vrUiWeaponLocker, 0, 0)) return false;
    if (!IsEquipPage()) return false;
    if (InterlockedCompareExchange(&g_weaponWheelOpen, 0, 0)) return false;
    if (InterlockedCompareExchange(&g_panelOpen, 0, 0)) return false;
    return true;
}

inline void PublishFromEngine() {
    if (!EngineCanInteract()) {
        LockerVrLoadoutPublish::Clear();
        return;
    }
    const uint32_t available = LockerVrLoadoutRuntime::AvailableOrdinaryMask(g_playerPawn);
    const uint32_t owned = NativeOwnedInventory::Mask();
    const uint32_t pending = LockerVrLoadoutPolicy::PendingMask(available, owned);
    const uint32_t uiQueued = static_cast<uint32_t>(InterlockedCompareExchange(&g_uiQueuedMask, 0, 0));
    LockerVrLoadoutPublish::Publish(available, owned, pending, true, uiQueued);

    const uintptr_t fn = LockerVrLoadoutAcquire::ResolveGameSystemFactoryFn();
    if (fn) LockerVrLoadoutIdentity::PinFactoryOffsetFromLiveSlot(fn);
}

inline void OnPopMenuScheduleAcquire() {
    if (!Enabled()) return;
    const uint32_t queued = static_cast<uint32_t>(InterlockedCompareExchange(&g_queuedMask, 0, 0));
    InterlockedExchange(&g_uiQueuedMask, 0);
    if (!queued) return;
    const uint32_t available = LockerVrLoadoutRuntime::AvailableOrdinaryMask(g_playerPawn);
    const uint32_t owned = NativeOwnedInventory::Mask();
    const uint32_t pendingNow = LockerVrLoadoutPolicy::RevalidateQueuedMask(queued, available, owned);
    if (!pendingNow) {
        Log("locker vr loadout: post-PopMenu queue dropped after revalidation (was 0x%X)", queued);
        PublishFromEngine();
        return;
    }
    InterlockedExchange(&g_postPopAcquireMask, static_cast<LONG>(pendingNow));
    InterlockedExchange(&g_acquireRequested, 1);
    Log("locker vr loadout: post-PopMenu scheduled acquire pendingNow=0x%X thread=%u", pendingNow,
        GetCurrentThreadId());
    PublishFromEngine();
}

inline void TryRunDeferredAcquire() {
    if (!Enabled()) return;
    if (!InterlockedCompareExchange(&g_acquireRequested, 0, 0)) return;
    if (InterlockedCompareExchange(&g_vrUiWeaponLocker, 0, 0)) return;
    if (InterlockedCompareExchange(&g_inCinematic, 0, 0)) return;
    if (InterlockedCompareExchange(&g_gamePaused, 0, 0)) return;
    if (InterlockedCompareExchange(&g_panelOpen, 0, 0)) return;

    InterlockedExchange(&g_acquireRequested, 0);
    const uint32_t requested =
        static_cast<uint32_t>(InterlockedExchange(&g_postPopAcquireMask, 0));
    if (!requested) return;

    const uint32_t available = LockerVrLoadoutRuntime::AvailableOrdinaryMask(g_playerPawn);
    const uint32_t ownedBefore = NativeOwnedInventory::Mask();
    const uint32_t pendingNow = LockerVrLoadoutPolicy::RevalidateQueuedMask(requested, available, ownedBefore);
    if (!pendingNow) {
        Log("locker vr loadout: deferred acquire dropped after revalidation (was 0x%X)", requested);
        PublishFromEngine();
        return;
    }
    Log("locker vr loadout: deferred acquire pendingNow=0x%X thread=%u", pendingNow, GetCurrentThreadId());
    LockerVrLoadoutAcquire::ApplyPendingMask(pendingNow, ownedBefore);
    PublishFromEngine();
}

inline void OnDefocus() {
    InterlockedExchange(&g_queuedMask, 0);
    InterlockedExchange(&g_uiQueuedMask, 0);
    InterlockedExchange(&g_pageSignal, 0);
    LockerVrLoadoutPublish::Clear();
}

struct ExecHooks {
    static void __thiscall Load(void* self, void* frame) {
        ++t_reenter;
        if (t_reenter == 1) InterlockedExchange(&g_pageSignal, 1);
        g_origLoad(self, frame);
        --t_reenter;
    }
    static void __thiscall Pop(void* self, void* frame) {
        ++t_reenter;
        g_origPop(self, frame);
        if (t_reenter == 1) OnPopMenuScheduleAcquire();
        --t_reenter;
    }
    static void __thiscall LoadUpgrades(void* self, void* frame, void* result) {
        ++t_reenter;
        if (t_reenter == 1) InterlockedExchange(&g_pageSignal, 2);
        g_origLoadUpgrades(self, frame, result);
        --t_reenter;
    }
};

inline bool VtSlot(uintptr_t vt, unsigned off, uintptr_t expect) {
    if (!vt || !PauseRttProbe::SafeRange(vt + off, 4)) return false;
    return PauseRttProbe::Pointer(vt + off) == expect;
}

inline bool InstallOne(uintptr_t target, void* detour, void** orig, const char* label) {
    if (target < g_textLo || target >= g_textHi || !PauseRttProbe::SafeRange(target, 8)) {
        Log("locker vr loadout: refuse hook %s target=%p", label, (void*)target);
        return false;
    }
    if (MH_CreateHook((void*)target, detour, orig) != MH_OK || MH_EnableHook((void*)target) != MH_OK) {
        Log("locker vr loadout: hook failed %s @ %p", label, (void*)target);
        return false;
    }
    return true;
}

inline void RollbackHooks() {
    if (g_origLoad) {
        MH_DisableHook((void*)kFnLoad);
        MH_RemoveHook((void*)kFnLoad);
        g_origLoad = nullptr;
    }
    if (g_origPop) {
        MH_DisableHook((void*)kFnPopMenu);
        MH_RemoveHook((void*)kFnPopMenu);
        g_origPop = nullptr;
    }
    if (g_origLoadUpgrades) {
        MH_DisableHook((void*)kFnLoadUpgrades);
        MH_RemoveHook((void*)kFnLoadUpgrades);
        g_origLoadUpgrades = nullptr;
    }
    InterlockedExchange(&g_hooksLive, 0);
}

inline bool TryInstallHooks() {
    if (!Enabled()) return false;
    if (InterlockedCompareExchange(&g_hooksLive, 0, 0)) return true;
    if (!g_textLo || !g_textHi) return false;
    const uintptr_t vt = kWeaponLockerVtable;
    if (!VtSlot(vt, kSlotLoad, kFnLoad) || !VtSlot(vt, kSlotPopMenu, kFnPopMenu) ||
        !VtSlot(vt, 0x1F8u, kFnEquip))
        return false;
    const bool ok = InstallOne(kFnLoad, (void*)&ExecHooks::Load, (void**)&g_origLoad, "LoadEquippedWeapons") &&
                    InstallOne(kFnPopMenu, (void*)&ExecHooks::Pop, (void**)&g_origPop, "PopMenu") &&
                    InstallOne(kFnLoadUpgrades, (void*)&ExecHooks::LoadUpgrades, (void**)&g_origLoadUpgrades,
                               "LoadUpgrades");
    if (!ok) {
        RollbackHooks();
        return false;
    }
    InterlockedExchange(&g_hooksLive, 1);
    Log("locker vr loadout: hooks live Load/PopMenu/LoadUpgrades (page gate + deferred acquire)");
    return true;
}

inline void TryInstallEarly() {
    if (Enabled()) TryInstallHooks();
}

inline bool HitEquipAll(float u, float v) {
    return u >= 0.73f && u <= 0.88f && v >= 0.54f && v <= 0.60f;
}

inline int HitTileIndex(float u, float v) {
    const float cu = 0.78f, cv = 0.47f, du = 0.055f, dv = 0.055f;
    const float tiles[4][2] = {{cu - du, cv - dv}, {cu + du, cv - dv}, {cu - du, cv + dv}, {cu + du, cv + dv}};
    for (int i = 0; i < 4; ++i) {
        if (u >= tiles[i][0] - 0.045f && u <= tiles[i][0] + 0.045f && v >= tiles[i][1] - 0.045f &&
            v <= tiles[i][1] + 0.045f)
            return i;
    }
    return -1;
}

inline bool PointerHitsModTarget(float u, float v) {
    if (!LockerVrLoadoutPublish::CanInteract()) return false;
    if (HitEquipAll(u, v) && LockerVrLoadoutPublish::Pending()) return true;
    const int tile = HitTileIndex(u, v);
    if (tile < 0) return false;
    static const uint32_t kBits[4] = {1u, 2u, 4u, 8u};
    const uint32_t bit = kBits[tile];
    const uint32_t avail = LockerVrLoadoutPublish::Available();
    const uint32_t owned = LockerVrLoadoutPublish::Owned();
    return (avail & bit) && !(owned & bit);
}

inline bool OnPointerSelect(float u, float v) {
    if (!LockerVrLoadoutPublish::CanInteract()) return false;
    if (HitEquipAll(u, v)) {
        if (!LockerVrLoadoutPublish::Pending()) return false;
        const uint32_t pending = LockerVrLoadoutPublish::Pending();
        InterlockedExchange(&g_queuedMask, static_cast<LONG>(pending));
        InterlockedExchange(&g_uiQueuedMask, static_cast<LONG>(pending));
        Log("locker vr loadout: EQUIP ALL queued pending=0x%X (XR path)", pending);
        return true;
    }
    const int tile = HitTileIndex(u, v);
    if (tile < 0) return false;
    static const uint32_t kBits[4] = {1u, 2u, 4u, 8u};
    const uint32_t bit = kBits[tile];
    const uint32_t avail = LockerVrLoadoutPublish::Available();
    const uint32_t owned = LockerVrLoadoutPublish::Owned();
    if ((avail & bit) == 0 || (owned & bit) != 0) return false;

    LONG prev = InterlockedCompareExchange(&g_queuedMask, 0, 0);
    if (prev & bit) {
        prev &= ~static_cast<LONG>(bit);
        InterlockedExchange(&g_queuedMask, prev);
        InterlockedExchange(&g_uiQueuedMask, prev);
        Log("locker vr loadout: tile unqueued vrBit=0x%X", bit);
    } else {
        prev |= static_cast<LONG>(bit);
        InterlockedExchange(&g_queuedMask, prev);
        InterlockedExchange(&g_uiQueuedMask, prev);
        Log("locker vr loadout: tile queued vrBit=0x%X", bit);
    }
    return true;
}

inline void PollEngine() {
    if (!Enabled()) return;
    TryInstallHooks();
    PublishFromEngine();
    TryRunDeferredAcquire();
}

} // namespace LockerVrLoadout
