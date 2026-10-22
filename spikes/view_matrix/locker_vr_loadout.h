#pragma once
#include "MinHook.h"
#include "locker_vr_loadout_acquire.h"
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
extern uintptr_t g_textLo, g_textHi;
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

inline volatile LONG g_pageSignal = 0; // 1=LoadEquippedWeapons, 2=LoadUpgrades
inline volatile LONG g_hooksLive = 0;
inline volatile LONG g_queuedMask = 0;
inline volatile LONG g_uiQueuedMask = 0; // visual QUEUED state
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

inline bool CanDrawOrInteract() {
    if (!Enabled()) return false;
    if (!InterlockedCompareExchange(&g_vrUiWeaponLocker, 0, 0)) return false;
    if (!IsEquipPage()) return false;
    if (InterlockedCompareExchange(&g_weaponWheelOpen, 0, 0)) return false;
    if (InterlockedCompareExchange(&g_panelOpen, 0, 0)) return false;
    return true;
}

inline uint32_t AvailableMask() { return LockerVrLoadoutRuntime::AvailableOrdinaryMask(g_playerPawn); }

inline uint32_t OwnedMask() { return NativeOwnedInventory::Mask(); }

inline uint32_t PendingMaskNow() {
    return LockerVrLoadoutPolicy::PendingMask(AvailableMask(), OwnedMask());
}

inline void QueueEquipAll() {
    const uint32_t pending = PendingMaskNow();
    if (!pending) return;
    InterlockedExchange(&g_queuedMask, static_cast<LONG>(pending));
    InterlockedExchange(&g_uiQueuedMask, static_cast<LONG>(pending));
    Log("locker vr loadout: EQUIP ALL queued pending=0x%X", pending);
}

inline void OnPopMenuComplete() {
    if (!Enabled()) return;
    const uint32_t queued = static_cast<uint32_t>(InterlockedCompareExchange(&g_queuedMask, 0, 0));
    InterlockedExchange(&g_uiQueuedMask, 0);
    if (!queued) return;
    const uint32_t pendingNow = LockerVrLoadoutPolicy::RevalidateQueuedMask(
        queued, AvailableMask(), OwnedMask());
    if (!pendingNow) {
        Log("locker vr loadout: post-PopMenu queue dropped after revalidation (was 0x%X)", queued);
        return;
    }
    Log("locker vr loadout: post-PopMenu apply pendingNow=0x%X thread=%u", pendingNow, GetCurrentThreadId());
    LockerVrLoadoutAcquire::ApplyPendingMask(pendingNow);
}

inline void OnDefocus() {
    InterlockedExchange(&g_queuedMask, 0);
    InterlockedExchange(&g_uiQueuedMask, 0);
    InterlockedExchange(&g_pageSignal, 0);
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
        if (t_reenter == 1) OnPopMenuComplete();
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
    Log("locker vr loadout: hooks live Load/PopMenu/LoadUpgrades (page gate + post-PopMenu apply)");
    return true;
}

inline void TryInstallEarly() {
    if (Enabled()) TryInstallHooks();
}

// Normalized UV hit tests (0..1). Panel center ~(0.78, 0.47), 2x2 tiles + EQUIP ALL button.
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

inline void OnPointerSelect(float u, float v) {
    if (!CanDrawOrInteract()) return;
    if (HitEquipAll(u, v)) {
        QueueEquipAll();
        return;
    }
    const int tile = HitTileIndex(u, v);
    if (tile < 0) return;
    static const uint32_t kBits[4] = {1u, 2u, 4u, 8u};
    const uint32_t bit = kBits[tile];
    const uint32_t avail = AvailableMask();
    const uint32_t owned = OwnedMask();
    if ((avail & bit) == 0 || (owned & bit) != 0) return;
    InterlockedExchange(&g_queuedMask, static_cast<LONG>(bit));
    InterlockedExchange(&g_uiQueuedMask, static_cast<LONG>(bit));
    Log("locker vr loadout: tile queued vrBit=0x%X", bit);
}

inline void PollInstall() { if (Enabled()) TryInstallHooks(); }

} // namespace LockerVrLoadout
