#pragma once
// Read-only availability probes for WeaponLocker diagnostic pass.
#include "inventory_retention_policy.h"
#include "native_owned_inventory.h"
#include "pause_rtt_probe.h"

void Log(_Printf_format_string_ const char* fmt, ...);

namespace LockerAvailabilityWitness {

inline int g_offWeaponsAvailableToLocker = -1;
inline int g_offSessionWeapons = -1;
inline int g_offEngineSessionManager = -1;

inline int EnsureWeaponsAvailableOffset(uintptr_t sampleItemClass) {
    if (g_offWeaponsAvailableToLocker >= 0) return g_offWeaponsAvailableToLocker;
    if (sampleItemClass)
        g_offWeaponsAvailableToLocker = PauseRttProbe::Property(
            sampleItemClass, "mWeaponsAvailableToLocker", "IntProperty");
    return g_offWeaponsAvailableToLocker;
}

inline void EnsureSessionLayout(uintptr_t engine) {
    if (g_offEngineSessionManager >= 0) return;
    if (!engine) return;
    const uintptr_t ec = PauseRttProbe::Pointer(engine + OBJ_CLASS);
    if (!ec) return;
    g_offEngineSessionManager = PauseRttProbe::Property(ec, "SessionManager", "ObjectProperty");
    if (g_offEngineSessionManager < 0) return;
    const uintptr_t sm = PauseRttProbe::Pointer(engine + (uintptr_t)g_offEngineSessionManager);
    const uintptr_t sc = sm ? PauseRttProbe::Pointer(sm + OBJ_CLASS) : 0;
    if (!sc) return;
    g_offSessionWeapons = PauseRttProbe::Property(sc, "mWeapons", "ArrayProperty");
}

inline void LogSessionWeaponsTable(const char* tag) {
    const uintptr_t engine = PauseRttProbe::Pointer(0x01CF8778u);
    EnsureSessionLayout(engine);
    if (g_offEngineSessionManager < 0 || !engine) {
        Log("locker avail witness: %s SessionManager unresolved", tag);
        return;
    }
    const uintptr_t sm = PauseRttProbe::Pointer(engine + (uintptr_t)g_offEngineSessionManager);
    if (!sm || g_offSessionWeapons < 0) {
        Log("locker avail witness: %s session=%p mWeapons off=%d", tag, (void*)sm, g_offSessionWeapons);
        return;
    }
    const uintptr_t arr = sm + (uintptr_t)g_offSessionWeapons;
    if (!PauseRttProbe::SafeRange(arr, 12)) return;
    const uintptr_t data = PauseRttProbe::Pointer(arr);
    const int count = *reinterpret_cast<const int*>(arr + 4);
    const int cap = *reinterpret_cast<const int*>(arr + 8);
    Log("locker avail witness: %s SessionManager=%p mWeapons count=%d cap=%d data=%p", tag, (void*)sm, count,
        cap, (void*)data);
}

inline void LogInventoryAvailability(const char* tag) {
    using namespace NativeOwnedInventory;
    const uintptr_t m = manager;
    if (!m) {
        Log("locker avail witness: %s inventory manager not cached (no Poll from hook)", tag);
        return;
    }
    constexpr uintptr_t kChain = 0x1e0;
    constexpr uintptr_t kWeaponName = 0x2e8;
    uintptr_t seen[16]{};
    unsigned visits = 0;
    for (uintptr_t it = PauseRttProbe::Pointer(m + kChain); it && visits < 16;
         it = PauseRttProbe::Pointer(it + kChain)) {
        for (unsigned k = 0; k < visits; ++k)
            if (seen[k] == it) return;
        seen[visits++] = it;
        const uintptr_t ic = PauseRttProbe::Pointer(it + OBJ_CLASS);
        const int off = EnsureWeaponsAvailableOffset(ic);
        if (off < 0) continue;
        if (!PauseRttProbe::SafeRange(it + (uintptr_t)off, 4)) continue;
        const int raw = *reinterpret_cast<const int*>(it + (uintptr_t)off);
        char wn[96]{};
        NameOf(it, wn, sizeof(wn));
        char fname[96]{};
        if (PauseRttProbe::SafeRange(it + kWeaponName, 8))
            NameFromIndex(*reinterpret_cast<const int*>(it + kWeaponName), fname, sizeof(fname));
        const int mesh = inventoryretention::WeaponMesh(fname[0] ? fname : wn);
        Log("locker avail witness: %s item=%p mesh=%d mWeaponName=%s mWeaponsAvailableToLocker=%d",
            tag, (void*)it, mesh, fname, raw);
    }
    if (EnsureWeaponsAvailableOffset(0) < 0)
        Log("locker avail witness: %s mWeaponsAvailableToLocker offset unresolved", tag);
}

} // namespace LockerAvailabilityWitness
