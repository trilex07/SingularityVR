#pragma once
// Archetype source: same as mission unlock @ 0xF03450 (GGameSystem vtable +0x20C, catalog row +0x0C FString).
// Pickup: RvPlayerPawnSP vtable +0xD70 keep=1 (witness @ 0x11B4BAA / inventory_pickup_policy.h).
#include "locker_vr_loadout_runtime.h"
#include "native_owned_inventory.h"
#include "pause_rtt_probe.h"
#include <intrin.h>

extern uintptr_t g_textLo, g_textHi;
extern uintptr_t g_playerPawn;
extern volatile LONG g_keepAcquiredWeapons;
void Log(_Printf_format_string_ const char* fmt, ...);

namespace InventoryVmHook {
int RunAuditedPickupRestore(void* pawn, void* archetype);
}

namespace LockerVrLoadoutAcquire {

constexpr uintptr_t kFnAppendFName = 0x4AFFD0u;
constexpr uintptr_t kGsVtableLookupOff = 0x20Cu;
constexpr uintptr_t kFnManagerHasArchetype = 0xF69590u;
constexpr unsigned kFactoryPrologueBytes = 16u;

inline volatile LONG g_factoryWitnessed = 0;
inline uintptr_t g_factoryFn = 0;
inline unsigned long long g_factoryPrologueHash = 0;

inline unsigned long long HashPrologue(uintptr_t fn) {
    if (!fn || !PauseRttProbe::SafeRange(fn, kFactoryPrologueBytes)) return 0;
    unsigned long long h = 1469598103934665603ull;
    const auto* p = reinterpret_cast<const unsigned char*>(fn);
    for (unsigned i = 0; i < kFactoryPrologueBytes; ++i) {
        h ^= p[i];
        h *= 1099511628211ull;
    }
    return h;
}

inline bool FactoryFnInText(uintptr_t fn) { return fn >= g_textLo && fn < g_textHi; }

inline uintptr_t ResolveGameSystemFactoryFn() {
    const uintptr_t gs = PauseRttProbe::Pointer(LockerVrLoadoutRuntime::kGGameSystemPtr);
    if (!gs || !PauseRttProbe::SafeRange(gs, 4)) return 0;
    const uintptr_t vt = PauseRttProbe::Pointer(gs);
    if (!PauseRttProbe::SafeRange(vt + kGsVtableLookupOff, 4)) return 0;
    const uintptr_t fn = PauseRttProbe::Pointer(vt + kGsVtableLookupOff);
    if (!FactoryFnInText(fn)) return 0;
    return fn;
}

inline bool WitnessFactoryOnce() {
    if (InterlockedCompareExchange(&g_factoryWitnessed, 0, 0)) return g_factoryFn != 0;
    const uintptr_t fn = ResolveGameSystemFactoryFn();
    if (!fn) {
        InterlockedExchange(&g_factoryWitnessed, 1);
        Log("locker vr loadout acquire: factory refused — GGameSystem vtable+0x20C unreadable");
        return false;
    }
    g_factoryFn = fn;
    g_factoryPrologueHash = HashPrologue(fn);
    InterlockedExchange(&g_factoryWitnessed, 1);
    Log("locker vr loadout acquire: factory pinned fn=%p prologueHash=%llX (witness 0xF03450 / row+0x0C)",
        (void*)fn, g_factoryPrologueHash);
    return true;
}

#if defined(_M_IX86)

using ManagerHasArchetypeFn = int(__thiscall*)(void* mgr, void* archetype);

inline void* LookupArchetypeViaNativeAbi(void* gameSystem, const void* fstringAtRowC) {
    if (!gameSystem || !fstringAtRowC) return nullptr;
    void* archetype = nullptr;
    __asm {
        push 1
        push 0
        sub esp, 8
        mov ecx, esp
        push 1
        push 1
        push fstringAtRowC
        mov eax, kFnAppendFName
        call eax
        mov ecx, gameSystem
        mov eax, dword ptr [ecx]
        mov eax, dword ptr [eax + kGsVtableLookupOff]
        call eax
        mov archetype, eax
        add esp, 16 // push(1)+push(0)+sub(8)
    }
    return archetype;
}

#else

inline void* LookupArchetypeViaNativeAbi(void*, const void*) { return nullptr; }

#endif

inline bool ArchetypeIsWeaponShared(void* archetype) {
    if (!archetype) return false;
    const uintptr_t cls = PauseRttProbe::Pointer(reinterpret_cast<uintptr_t>(archetype) + OBJ_CLASS);
    if (!cls) return false;
    char name[96]{};
    return NameOf(cls, name, sizeof(name)) && !strcmp(name, "RvWeaponShared");
}

inline bool ManagerAlreadyHas(void* mgr, void* archetype) {
    if (!mgr || !archetype) return true;
    if (!FactoryFnInText(kFnManagerHasArchetype)) return true;
    const auto fn = reinterpret_cast<ManagerHasArchetypeFn>(kFnManagerHasArchetype);
    __try {
        return fn(mgr, archetype) != 0;
    } __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER
                                                                 : EXCEPTION_CONTINUE_SEARCH) {
        return true;
    }
}

inline bool TryPickupOrdinary(uint32_t vrBit, int* pickupResult) {
    if (pickupResult) *pickupResult = -1;
    if (!WitnessFactoryOnce() || !g_factoryFn) return false;

    const unsigned index = LockerVrLoadoutPolicy::CatalogIndexForVrBit(vrBit);
    if (index == ~0u) return false;
    LockerVrLoadoutPolicy::CatalogRowView row{};
    if (!LockerVrLoadoutRuntime::ReadCatalogRow(index, &row) ||
        !LockerVrLoadoutPolicy::RowEligible(row))
        return false;

    const uintptr_t pawn = g_playerPawn;
    if (!pawn || !PauseRttProbe::SafeRange(pawn, 4)) return false;
    const uintptr_t mgr = LockerVrLoadoutRuntime::ResolveInventoryManager(pawn);
    if (!mgr) return false;

    const void* fstr = LockerVrLoadoutRuntime::FStringPtrAtCatalogRowSecondary(index);
    if (!fstr) return false;

    void* gs = reinterpret_cast<void*>(PauseRttProbe::Pointer(LockerVrLoadoutRuntime::kGGameSystemPtr));
    void* archetype = LookupArchetypeViaNativeAbi(gs, fstr);
    if (!ArchetypeIsWeaponShared(archetype)) return false;
    if (ManagerAlreadyHas(reinterpret_cast<void*>(mgr), archetype)) return false;

    const uint32_t ownedBefore = NativeOwnedInventory::Mask();
    const int result = InventoryVmHook::RunAuditedPickupRestore(reinterpret_cast<void*>(pawn), archetype);
    if (pickupResult) *pickupResult = result;
    NativeOwnedInventory::Refresh();
    const uint32_t ownedAfter = NativeOwnedInventory::Mask();
    const bool success = (ownedAfter & vrBit) != 0 && (ownedBefore & vrBit) == 0;

    static unsigned receipts = 0;
    if (receipts++ < 8) {
        const char* weapon = LockerVrLoadoutPolicy::WeaponLogNameForVrBit(vrBit);
        Log("locker vr loadout acquire: weapon=%s vrBit=0x%X available=1 ownedBefore=%d pickup=%s ownedAfter=%d "
            "factory=%p archetype=%p ret=%d",
            weapon ? weapon : "?", vrBit, (ownedBefore & vrBit) ? 1 : 0, success ? "success" : "fail",
            (ownedAfter & vrBit) ? 1 : 0, (void*)g_factoryFn, archetype, result);
    }
    return success;
}

inline void ApplyPendingMask(uint32_t pendingNow) {
    if (!pendingNow) return;
    if (!InterlockedCompareExchange(&g_keepAcquiredWeapons, 0, 0)) return;
    for (uint32_t bit = 1; bit <= 8; bit <<= 1) {
        if ((pendingNow & bit) == 0) continue;
        TryPickupOrdinary(bit, nullptr);
    }
    NativeOwnedInventory::Refresh();
}

} // namespace LockerVrLoadoutAcquire
