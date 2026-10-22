#pragma once
#include <Windows.h>
#include <cstdint>

namespace LockerRetainPolicy {

constexpr uintptr_t kFnDestroyItem = 0x00A05790u;
constexpr uintptr_t kPopMenuDestroyCallSite = 0x00F18059u;
constexpr uintptr_t kPopMenuDestroyReturn = 0x00F1805Eu; // byte immediately after E8 rel32
// PopMenu @ 0xF1804F: mov ecx, [0x1CF74E4] before destroyItem.
constexpr uintptr_t kPopMenuInventoryGlobal = 0x01CF74E4u;

// destroyItem @ 0xA05790: __thiscall, ret 0xC (capture-20261006-205508).
// ECX = inventory manager from kPopMenuInventoryGlobal at the PopMenu callsite.
// Stack: item @ [ebp+8], arg2 @ [ebp+0xC] (PopMenu=0), arg3 @ [ebp+0x10] (PopMenu=1).

inline bool CallerIsPopMenuDestroy(uintptr_t retAddr) {
    return retAddr == kPopMenuDestroyReturn;
}

inline bool EstablishedThreadMatches(DWORD establishedThread, DWORD currentThread) {
    if (!establishedThread) return false;
    return establishedThread == currentThread;
}

inline bool PopMenuManagerMatches(uintptr_t callManager, uintptr_t globalManager) {
    if (!callManager || !globalManager) return false;
    return callManager == globalManager;
}

inline bool PopMenuDestroyArgsMatch(int arg2, int arg3) {
    return arg2 == 0 && arg3 == 1;
}

inline bool ShouldSuppress(bool lockerRetainOrdinary, bool keepAcquiredWeapons, uintptr_t retAddr,
                            DWORD establishedThread, DWORD currentThread, uintptr_t callManager,
                            uintptr_t globalManager, int arg2, int arg3, bool managerClassOk,
                            bool itemReadable, bool weaponSharedClass, bool supportedOrdinaryWeapon) {
    if (!lockerRetainOrdinary || !keepAcquiredWeapons) return false;
    if (!CallerIsPopMenuDestroy(retAddr)) return false;
    if (!EstablishedThreadMatches(establishedThread, currentThread)) return false;
    if (!PopMenuManagerMatches(callManager, globalManager)) return false;
    if (!PopMenuDestroyArgsMatch(arg2, arg3)) return false;
    if (!managerClassOk) return false;
    if (!itemReadable || !weaponSharedClass || !supportedOrdinaryWeapon) return false;
    return true;
}

} // namespace LockerRetainPolicy
