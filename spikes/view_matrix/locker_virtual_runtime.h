#pragma once
#include "locker_virtual_contract.h"
#include "locker_station_grant.h"
#include "native_owned_inventory.h"
#include "pause_rtt_probe.h"

extern volatile LONG g_lockerVirtualOrdinary;
extern volatile LONG g_lockerVirtualApply;
void Log(_Printf_format_string_ const char* fmt, ...);

namespace LockerVirtualRuntime {

inline bool Enabled() {
    return InterlockedCompareExchange(&g_lockerVirtualOrdinary, 0, 0) != 0;
}

inline bool ApplyEnabled() {
    return Enabled() && InterlockedCompareExchange(&g_lockerVirtualApply, 0, 0) != 0;
}

inline void LogPlan(unsigned mask, const lockervirtual::VirtualLoadView& plan, const char* tag) {
    static unsigned notes = 0;
    if (notes++ >= 12) return;
    Log("locker virtual: plan uiOrdinary=%d controllerOrdinary=%d mask=0x%X (%s)",
        plan.uiOrdinaryCount, plan.controllerOrdinaryCount, mask, tag);
    for (int i = 0; i < plan.uiOrdinaryCount && i < lockervirtual::kMaxOrdinaryMeshes; ++i)
        Log("locker virtual: ui[%d] mesh=%d ordinary=%d", i, plan.uiMeshes[i],
            lockervirtual::OrdinaryMesh(plan.uiMeshes[i]) ? 1 : 0);
}

// UE3 TArray<FName>: data, count, max (8-byte FName entries).
inline bool WriteFNameArray(uintptr_t movie, int propOff, const NativeOwnedInventory::Name* names, int n,
                            const char* label) {
    if (propOff < 0 || n < 0 || n > lockervirtual::kMaxOrdinaryMeshes) return false;
    const uintptr_t arr = movie + (uintptr_t)propOff;
    if (!PauseRttProbe::SafeRange(arr, 12)) return false;
    const uintptr_t data = *reinterpret_cast<uintptr_t*>(arr);
    const int before = *reinterpret_cast<int*>(arr + 4);
    const int cap = *reinterpret_cast<int*>(arr + 8);
    if (n > cap || cap > 256 || cap < 0) {
        Log("locker virtual: apply %s refused capacity=%d want=%d", label, cap, n);
        return false;
    }
    if (n > 0 && (!data || !PauseRttProbe::SafeRange(data, (size_t)n * 8))) {
        Log("locker virtual: apply %s refused data=%p count=%d", label, (void*)data, n);
        return false;
    }
    for (int i = 0; i < n; ++i)
        *reinterpret_cast<NativeOwnedInventory::Name*>(data + (uintptr_t)i * 8) = names[i];
    *reinterpret_cast<int*>(arr + 4) = n;
    Log("locker virtual: apply %s count %d -> %d (cap=%d; PopMenu still phase D)", label, before, n, cap);
    return true;
}

inline void OnWeaponLockerFocus(uintptr_t movie, uintptr_t cls) {
    if (InterlockedCompareExchange(&g_lockerStationGrantOrdinary, 0, 0) != 0)
        LockerStationGrantOrdinaryTry(movie, cls);
    if (!Enabled()) return;
    NativeOwnedInventory::Refresh();
    if (!NativeOwnedInventory::SnapshotNow() || !NativeOwnedInventory::Ready()) {
        static unsigned emptyNotes = 0;
        if (emptyNotes++ < 4)
            Log("locker virtual: owned inventory not ready; no virtual plan/apply");
        return;
    }
    const unsigned mask = NativeOwnedInventory::Mask();
    const lockervirtual::VirtualLoadView plan = lockervirtual::PlanVirtualLoad(mask);
    LogPlan(mask, plan, ApplyEnabled() ? "phase C TArray apply" : "phase B read-only");

    if (!ApplyEnabled() || plan.uiOrdinaryCount <= 0) return;

    NativeOwnedInventory::Name names[lockervirtual::kMaxOrdinaryMeshes]{};
    for (int i = 0; i < plan.uiOrdinaryCount; ++i) {
        if (!NativeOwnedInventory::WeaponIdent(plan.uiMeshes[i], &names[i])) {
            Log("locker virtual: apply refused missing FName for mesh=%d", plan.uiMeshes[i]);
            return;
        }
    }

    static uintptr_t lastMovie = 0;
    static unsigned applyNotes = 0;
    if (movie == lastMovie && applyNotes >= 2) return;
    if (movie != lastMovie) {
        lastMovie = movie;
        applyNotes = 0;
    }
    ++applyNotes;

    const char* fields[] = {"mEquippedWeapons", "mOriginalWeapons"};
    for (const char* field : fields) {
        const int off = PauseRttProbe::Property(cls, field, "ArrayProperty");
        if (off < 0) {
            Log("locker virtual: apply %s unresolved (fail-closed)", field);
            return;
        }
        if (!WriteFNameArray(movie, off, names, plan.uiOrdinaryCount, field)) return;
    }
}

} // namespace LockerVirtualRuntime
