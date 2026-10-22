#pragma once
#include <cstdint>

namespace LockerRetainPolicy {

constexpr uintptr_t kFnDestroyItem = 0x00A05790u;
constexpr uintptr_t kPopMenuDestroyCallSite = 0x00F18059u;
constexpr uintptr_t kPopMenuDestroyReturn = 0x00F1805Eu; // byte immediately after E8 rel32

// destroyItem @ 0xA05790: __thiscall, ret 0xC (capture-20261006-205508).
// ECX = inventory manager (PopMenu loads [0x1CF74E4] before the call).
// Stack: item @ [ebp+8], arg2 @ [ebp+0xC] (PopMenu passes 0), arg3 @ [ebp+0x10] (PopMenu passes 1).
// Early/no-op paths return eax=0; PopMenu @ 0xF1805E does not read eax (inc edi only).

inline bool CallerIsPopMenuDestroy(uintptr_t retAddr) {
    return retAddr == kPopMenuDestroyReturn;
}

inline bool EstablishedThreadMatches(DWORD establishedThread, DWORD currentThread) {
    if (!establishedThread) return false;
    return establishedThread == currentThread;
}

inline bool ManagerMatches(uintptr_t callManager, uintptr_t establishedManager) {
    if (!establishedManager || !callManager) return false;
    return callManager == establishedManager;
}

inline bool ShouldSuppress(bool lockerRetainOrdinary, bool keepAcquiredWeapons, uintptr_t retAddr,
                            DWORD establishedThread, DWORD currentThread, uintptr_t callManager,
                            uintptr_t establishedManager, bool itemReadable, bool weaponSharedClass,
                            bool supportedOrdinaryWeapon) {
    if (!lockerRetainOrdinary || !keepAcquiredWeapons) return false;
    if (!CallerIsPopMenuDestroy(retAddr)) return false;
    if (!EstablishedThreadMatches(establishedThread, currentThread)) return false;
    if (!ManagerMatches(callManager, establishedManager)) return false;
    if (!itemReadable || !weaponSharedClass || !supportedOrdinaryWeapon) return false;
    return true;
}

} // namespace LockerRetainPolicy
