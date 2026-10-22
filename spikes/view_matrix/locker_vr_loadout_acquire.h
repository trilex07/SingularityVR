#pragma once
#include "locker_vr_loadout_identity.h"
#include "locker_vr_loadout_runtime.h"
#include "native_owned_inventory.h"
#include "pause_rtt_probe.h"
#include <intrin.h>

extern uintptr_t g_textLo, g_textHi;
extern uintptr_t g_playerPawn;
extern volatile LONG g_keepAcquiredWeapons;
extern volatile LONG g_inCinematic;
void Log(_Printf_format_string_ const char* fmt, ...);

namespace InventoryVmHook {
int RunAuditedPickupRestore(void* pawn, void* archetype);
}

namespace LockerVrLoadoutAcquire {

constexpr uintptr_t kFnAppendFName = 0x4AFFD0u;
constexpr uintptr_t kGsVtableLookupOff = 0x20Cu;

inline volatile LONG g_factoryWitnessed = 0;
inline uintptr_t g_factoryFn = 0;

inline uintptr_t ResolveGameSystemFactoryFn() {
    const uintptr_t gs = PauseRttProbe::Pointer(LockerVrLoadoutRuntime::kGGameSystemPtr);
    if (!LockerVrLoadoutIdentity::GameSystemObjectIdentityOk(gs)) return 0;
    const uintptr_t vt = PauseRttProbe::Pointer(gs);
    if (!PauseRttProbe::SafeRange(vt + kGsVtableLookupOff, 4)) return 0;
    const uintptr_t fn = PauseRttProbe::Pointer(vt + kGsVtableLookupOff);
    if (!LockerVrLoadoutIdentity::FactoryIdentityOk(fn)) return 0;
    return fn;
}

inline bool WitnessFactoryOnEngineThread() {
    if (InterlockedCompareExchange(&g_factoryWitnessed, 0, 0)) {
        if (g_factoryFn && LockerVrLoadoutIdentity::FactoryIdentityOk(g_factoryFn)) return true;
        InterlockedExchange(&g_factoryWitnessed, 0);
        g_factoryFn = 0;
    }
    if (!LockerVrLoadoutIdentity::AppendFNameIdentityOk()) {
        Log("locker vr loadout acquire: AppendFName identity refused (0x4AFFD0 prologue)");
        return false;
    }
    const uintptr_t fn = ResolveGameSystemFactoryFn();
    if (!fn) {
        Log("locker vr loadout acquire: factory identity refused (offline vtable+0x20C gate)");
        return false;
    }
    g_factoryFn = fn;
    InterlockedExchange(&g_factoryWitnessed, 1);
    Log("locker vr loadout acquire: factory verified fn=%p rva=0x%X (0xF03450 / 0x11B4B6E)",
        (void*)fn, (unsigned)LockerVrLoadoutIdentity::kExpectedFactoryRvaFromImage);
    return true;
}

#if defined(_M_IX86)

inline void* LookupArchetypeViaNativeAbi(void* gameSystem, const void* fstringAtRowC) {
    if (!gameSystem || !fstringAtRowC || !LockerVrLoadoutIdentity::AppendFNameIdentityOk()) return nullptr;
    const uintptr_t append = LockerVrLoadoutIdentity::ImageBase() + LockerVrLoadoutIdentity::kAppendFNameRvaFromImage;
    void* archetype = nullptr;
    __asm {
        push 1
        push 0
        sub esp, 8
        mov ecx, esp
        push 1
        push 1
        push fstringAtRowC
        mov eax, append
        call eax
        mov ecx, gameSystem
        mov eax, dword ptr [ecx]
        mov eax, dword ptr [eax + kGsVtableLookupOff]
        call eax
        mov archetype, eax
        add esp, 16
    }
    return archetype;
}

#else

inline void* LookupArchetypeViaNativeAbi(void*, const void*) { return nullptr; }

#endif

inline bool ArchetypeInWeaponSharedHierarchy(void* archetype) {
    if (!archetype) return false;
    const uintptr_t cls = PauseRttProbe::Pointer(reinterpret_cast<uintptr_t>(archetype) + OBJ_CLASS);
    return cls && NativeOwnedInventory::ClassHas(cls, "RvWeaponShared");
}

inline bool TryPickupOrdinary(uint32_t vrBit, int* pickupResult) {
    if (pickupResult) *pickupResult = -1;
    if (!WitnessFactoryOnEngineThread() || !LockerVrLoadoutIdentity::FactoryIdentityOk(g_factoryFn)) return false;

    const unsigned index = LockerVrLoadoutPolicy::CatalogIndexForVrBit(vrBit);
    if (index == ~0u) return false;
    LockerVrLoadoutPolicy::CatalogRowView row{};
    if (!LockerVrLoadoutRuntime::ReadCatalogRow(index, &row) ||
        !LockerVrLoadoutPolicy::RowEligible(row))
        return false;

    const uintptr_t pawn = g_playerPawn;
    if (!pawn || !PauseRttProbe::SafeRange(pawn, 4)) return false;
    if (!LockerVrLoadoutRuntime::ResolveInventoryManager(pawn)) return false;
    if (NativeOwnedInventory::Mask() & vrBit) return false;

    const void* fstr = LockerVrLoadoutRuntime::FStringPtrAtCatalogRowSecondary(index);
    if (!fstr) return false;

    void* gs = reinterpret_cast<void*>(PauseRttProbe::Pointer(LockerVrLoadoutRuntime::kGGameSystemPtr));
    void* archetype = LookupArchetypeViaNativeAbi(gs, fstr);
    if (!ArchetypeInWeaponSharedHierarchy(archetype)) return false;

    const int result = InventoryVmHook::RunAuditedPickupRestore(reinterpret_cast<void*>(pawn), archetype);
    if (pickupResult) *pickupResult = result;

    static unsigned receipts = 0;
    if (receipts++ < 8) {
        const char* weapon = LockerVrLoadoutPolicy::WeaponLogNameForVrBit(vrBit);
        Log("locker vr loadout acquire: pickup call weapon=%s vrBit=0x%X archetype=%p ret=%d",
            weapon ? weapon : "?", vrBit, archetype, result);
    }
    return result >= 0;
}

inline void ApplyPendingMask(uint32_t pendingNow, uint32_t ownedBefore) {
    if (!pendingNow) return;
    if (!InterlockedCompareExchange(&g_keepAcquiredWeapons, 0, 0)) return;
    if (InterlockedCompareExchange(&g_inCinematic, 0, 0)) return;

    uint32_t pickupCalled = 0;
    for (uint32_t bit = 1; bit <= 8; bit <<= 1) {
        if ((pendingNow & bit) == 0) continue;
        if (ownedBefore & bit) continue;
        int pr = -1;
        if (TryPickupOrdinary(bit, &pr)) pickupCalled |= bit;
    }

    const bool snap = NativeOwnedInventory::SnapshotNow();
    const uint32_t ownedAfter = snap ? NativeOwnedInventory::Mask() : 0;
    const uint32_t success = snap ? (pendingNow & ownedAfter & ~ownedBefore) : 0;
    const uint32_t failed = snap ? (pendingNow & ~ownedAfter & ~ownedBefore) : pendingNow;

    if (snap) {
        Log("locker vr loadout acquire batch: requested=0x%X pickupCalled=0x%X ownedBefore=0x%X ownedAfter=0x%X "
            "success=0x%X failed=0x%X",
            pendingNow, pickupCalled, ownedBefore, ownedAfter, success, failed);
        if (success) NativeOwnedInventory::Refresh();
    } else {
        Log("locker vr loadout acquire batch: requested=0x%X pickupCalled=0x%X ownedBefore=0x%X ownedAfter=UNKNOWN "
            "(SnapshotNow failed; Refresh queued)",
            pendingNow, pickupCalled, ownedBefore);
        NativeOwnedInventory::Refresh();
    }
}

} // namespace LockerVrLoadoutAcquire
