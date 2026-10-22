#pragma once
// Phase F: suppress PopMenu destroyItem for supported ordinary weapons when retention is enabled.
// Default OFF (Input LockerRetainOrdinary=0). No TArray / spawn / LoadEquipped changes.
#include "MinHook.h"
#include "inventory_retention_policy.h"
#include "locker_retain_policy.h"
#include "native_owned_inventory.h"
#include "pause_rtt_probe.h"
#include <intrin.h>

extern uintptr_t g_textLo, g_textHi;
extern volatile LONG g_keepAcquiredWeapons;
extern volatile LONG g_lockerRetainOrdinary;
void Log(_Printf_format_string_ const char* fmt, ...);

namespace LockerRetainOrdinary {

using DestroyItemFn = int(__thiscall*)(void* inv, void* item, int arg2, int arg3);

inline volatile LONG g_hookLive = 0;
inline DestroyItemFn g_origDestroy = nullptr;

constexpr unsigned kMaxSuppressLog = 16u;
constexpr unsigned kMaxRejectLog = 24u;
inline volatile LONG g_suppressLog = 0;
inline volatile LONG g_rejectLog = 0;

inline bool Enabled() {
    return InterlockedCompareExchange(&g_lockerRetainOrdinary, 0, 0) != 0 &&
           InterlockedCompareExchange(&g_keepAcquiredWeapons, 0, 0) != 0;
}

inline int ForwardDestroy(void* inv, void* item, int arg2, int arg3) {
    if (!g_origDestroy) {
        static unsigned nullForward = 0;
        if (nullForward++ < 4)
            Log("locker retention: forward destroy refused — orig trampoline null thread=%u",
                GetCurrentThreadId());
        return 0;
    }
    return g_origDestroy(inv, item, arg2, arg3);
}

inline bool TryLogReject(uintptr_t callerRet, void* item, const char* weapon, const char* reason) {
    if (InterlockedIncrement(&g_rejectLog) > (LONG)kMaxRejectLog) {
        InterlockedDecrement(&g_rejectLog);
        return false;
    }
    Log("locker retention: forward destroy caller=%p item=%p weapon=%s reason=%s thread=%u",
        (void*)callerRet, item, weapon && weapon[0] ? weapon : "(none)", reason, GetCurrentThreadId());
    return true;
}

inline bool TryLogSuppress(uintptr_t callerRet, void* item, const char* weapon, void* callManager, void* globalManager,
                            const char* mgrObjectName, const char* mgrClassName, void* mgrClassPtr) {
    if (InterlockedIncrement(&g_suppressLog) > (LONG)kMaxSuppressLog) {
        InterlockedDecrement(&g_suppressLog);
        return false;
    }
    const LONG n = InterlockedCompareExchange(&g_suppressLog, 0, 0);
    if (n <= 4)
        Log("locker retention: suppress destroy caller=%p item=%p weapon=%s manager=%p globalManager=%p "
            "mgrName=%s mgrClass=%s mgrClassPtr=%p thread=%u",
            (void*)callerRet, item, weapon, callManager, globalManager,
            mgrObjectName && mgrObjectName[0] ? mgrObjectName : "(none)",
            mgrClassName && mgrClassName[0] ? mgrClassName : "(none)", mgrClassPtr, GetCurrentThreadId());
    else
        Log("locker retention: suppress destroy caller=%p item=%p weapon=%s thread=%u", (void*)callerRet, item,
            weapon, GetCurrentThreadId());
    return true;
}

inline uintptr_t ReadPopMenuGlobalManager() {
    const uintptr_t slot = LockerRetainPolicy::kPopMenuInventoryGlobal;
    if (!PauseRttProbe::SafeRange(slot, 4)) return 0;
    return PauseRttProbe::Pointer(slot);
}

inline bool ClassWeaponShared(uintptr_t item) {
    using namespace NativeOwnedInventory;
    const uintptr_t cls = PauseRttProbe::Pointer(item + OBJ_CLASS);
    return cls && ClassHas(cls, "RvWeaponShared");
}

inline bool SupportedOrdinaryFromItem(uintptr_t item, char* outWeapon, unsigned outLen) {
    using namespace NativeOwnedInventory;
    if (!outWeapon || outLen < 2) return false;
    outWeapon[0] = '\0';
    if (!PauseRttProbe::SafeRange(item + kItemWeaponName, 8)) return false;
    char wn[96]{};
    NameFromIndex(*(const int*)(item + kItemWeaponName), wn, sizeof(wn));
    int mesh = inventoryretention::WeaponMesh(wn);
    if (!mesh) {
        char own[96]{};
        NameOf(item, own, sizeof(own));
        mesh = inventoryretention::WeaponMesh(own);
        if (mesh && own[0]) {
            strncpy_s(outWeapon, outLen, own, _TRUNCATE);
            return true;
        }
        if (wn[0]) strncpy_s(outWeapon, outLen, wn, _TRUNCATE);
        return false;
    }
    strncpy_s(outWeapon, outLen, wn[0] ? wn : "?", _TRUNCATE);
    return true;
}

struct DestroyHooks {
    static int __thiscall Detour(void* inv, void* item, int arg2, int arg3) {
        if (!Enabled()) return ForwardDestroy(inv, item, arg2, arg3);

        const uintptr_t ret = (uintptr_t)_ReturnAddress();
        const DWORD tid = GetCurrentThreadId();
        const DWORD established = NativeOwnedInventory::thread;
        const uintptr_t callMgr = (uintptr_t)inv;

        if (!LockerRetainPolicy::CallerIsPopMenuDestroy(ret))
            return ForwardDestroy(inv, item, arg2, arg3);

        uintptr_t globalMgr = 0;
        char weapon[96]{};
        char mgrObjectName[96]{};
        char mgrClassName[96]{};
        void* mgrClassPtr = nullptr;
        bool itemOk = false;
        bool shared = false;
        bool ordinary = false;
        __try {
            globalMgr = ReadPopMenuGlobalManager();
            if (callMgr) {
                using namespace NativeOwnedInventory;
                NameOf(callMgr, mgrObjectName, sizeof(mgrObjectName));
                const uintptr_t cls = PauseRttProbe::Pointer(callMgr + OBJ_CLASS);
                mgrClassPtr = (void*)cls;
                if (cls) NameOf(cls, mgrClassName, sizeof(mgrClassName));
            }
            const uintptr_t it = (uintptr_t)item;
            itemOk = it && PauseRttProbe::SafeRange(it, 8);
            if (itemOk) {
                shared = ClassWeaponShared(it);
                if (shared) ordinary = SupportedOrdinaryFromItem(it, weapon, sizeof(weapon));
            }
        } __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER
                                                                     : EXCEPTION_CONTINUE_SEARCH) {
            itemOk = false;
            globalMgr = 0;
        }

        const bool keep = InterlockedCompareExchange(&g_keepAcquiredWeapons, 0, 0) != 0;
        const bool retain = InterlockedCompareExchange(&g_lockerRetainOrdinary, 0, 0) != 0;
        if (LockerRetainPolicy::ShouldSuppress(retain, keep, ret, established, tid, callMgr, globalMgr, arg2, arg3,
                                               itemOk, shared, ordinary)) {
            TryLogSuppress(ret, item, weapon, inv, (void*)globalMgr, mgrObjectName, mgrClassName, mgrClassPtr);
            NativeOwnedInventory::Refresh();
            return 0;
        }

        if (!established) TryLogReject(ret, item, weapon, "no-established-thread");
        else if (!LockerRetainPolicy::EstablishedThreadMatches(established, tid))
            TryLogReject(ret, item, weapon, "thread");
        else if (!globalMgr) TryLogReject(ret, item, weapon, "no-popmenu-manager");
        else if (!callMgr || !LockerRetainPolicy::PopMenuManagerMatches(callMgr, globalMgr))
            TryLogReject(ret, item, weapon, "manager");
        else if (!LockerRetainPolicy::PopMenuDestroyArgsMatch(arg2, arg3))
            TryLogReject(ret, item, weapon, "args");
        else if (!itemOk) TryLogReject(ret, item, weapon, "item");
        else if (!shared) TryLogReject(ret, item, weapon, "not-weapon-shared");
        else if (!ordinary) TryLogReject(ret, item, weapon, "not-ordinary");

        return ForwardDestroy(inv, item, arg2, arg3);
    }
};

inline void DisableHook() {
    if (!g_origDestroy) return;
    MH_DisableHook((void*)LockerRetainPolicy::kFnDestroyItem);
    MH_RemoveHook((void*)LockerRetainPolicy::kFnDestroyItem);
    g_origDestroy = nullptr;
    InterlockedExchange(&g_hookLive, 0);
}

inline bool TryInstall() {
    if (!Enabled()) return false;
    if (InterlockedCompareExchange(&g_hookLive, 0, 0) != 0) return true;
    if (!g_textLo || !g_textHi || g_textLo >= g_textHi) return false;
    const uintptr_t target = LockerRetainPolicy::kFnDestroyItem;
    if (target < g_textLo || target >= g_textHi || !PauseRttProbe::SafeRange(target, 8)) return false;
    if (g_origDestroy) return true;
    const MH_STATUS c = MH_CreateHook((void*)target, (void*)&DestroyHooks::Detour, (void**)&g_origDestroy);
    if (c != MH_OK) {
        Log("locker retention: MH_CreateHook destroyItem status=%d", (int)c);
        return false;
    }
    const MH_STATUS e = MH_EnableHook((void*)target);
    if (e != MH_OK) {
        MH_RemoveHook((void*)target);
        g_origDestroy = nullptr;
        Log("locker retention: MH_EnableHook destroyItem status=%d", (int)e);
        return false;
    }
    InterlockedExchange(&g_hookLive, 1);
    Log("locker retention: destroyItem hook live @ %p (PopMenu ret gate %p global %p; LockerRetainOrdinary)",
        (void*)target, (void*)LockerRetainPolicy::kPopMenuDestroyReturn,
        (void*)LockerRetainPolicy::kPopMenuInventoryGlobal);
    return true;
}

} // namespace LockerRetainOrdinary
