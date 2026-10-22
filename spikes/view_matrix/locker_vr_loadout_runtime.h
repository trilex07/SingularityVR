#pragma once
#include "locker_vr_loadout_identity.h"
#include "locker_vr_loadout_policy.h"
#include "native_owned_inventory.h"
#include "pause_rtt_probe.h"
#include <cstring>

namespace LockerVrLoadoutRuntime {

constexpr uintptr_t kGGameSystemPtr = 0x01CF394Cu;
constexpr uintptr_t kCatalogDataOff = 0x160u;
constexpr uintptr_t kCatalogCountOff = 0x164u;
constexpr uintptr_t kCatalogStride = 0x50u;
constexpr uintptr_t kPawnInvManager = 0x3D0u;
constexpr uintptr_t kInventoryManagerVtable = 0x019CB480u;
constexpr uintptr_t kMgrNativeMaskOff = 0x3E4u;
constexpr uintptr_t kDefaultEmptyFString = 0x179E4D0u;

inline uintptr_t RelocatedDataAddress(uintptr_t pinnedVa) {
    return LockerVrLoadoutIdentity::ImageBase() + (pinnedVa - LockerVrLoadoutIdentity::kCaptureImageBase);
}

inline uintptr_t GameSystemInstance() { return PauseRttProbe::Pointer(RelocatedDataAddress(kGGameSystemPtr)); }

inline uintptr_t ExpectedInventoryManagerVtable() {
    return RelocatedDataAddress(kInventoryManagerVtable);
}

inline bool ReadCatalogRow(unsigned index, LockerVrLoadoutPolicy::CatalogRowView* out) {
    if (!out) return false;
    *out = {};
    const uintptr_t gs = GameSystemInstance();
    if (!gs || !PauseRttProbe::SafeRange(gs + kCatalogCountOff, 8)) return false;
    const int count = *reinterpret_cast<const int*>(gs + kCatalogCountOff);
    if (index >= static_cast<unsigned>(count)) return false;
    const uintptr_t data = PauseRttProbe::Pointer(gs + kCatalogDataOff);
    if (!data) return false;
    const uintptr_t row = data + static_cast<uintptr_t>(index) * kCatalogStride;
    if (!PauseRttProbe::SafeRange(row, kCatalogStride)) return false;
    out->row24 = *reinterpret_cast<const uint8_t*>(row + 0x24);
    out->row4c = *reinterpret_cast<const uint32_t*>(row + 0x4C);
    out->readable = true;
    return true;
}

inline bool RowAtCallback(unsigned index, LockerVrLoadoutPolicy::CatalogRowView* out) {
    return ReadCatalogRow(index, out);
}

inline uintptr_t ResolveInventoryManager(uintptr_t pawn) {
    if (!pawn || !PauseRttProbe::SafeRange(pawn + kPawnInvManager, 4)) return 0;
    const uintptr_t mgr = PauseRttProbe::Pointer(pawn + kPawnInvManager);
    if (!mgr || !PauseRttProbe::SafeRange(mgr, 4)) return 0;
    const uintptr_t vt = PauseRttProbe::Pointer(mgr);
    const uintptr_t expectedVt = ExpectedInventoryManagerVtable();
    if (!PauseRttProbe::SafeRange(vt, 4) || vt != expectedVt) return 0;
    return mgr;
}

inline uint32_t ReadNativeProgressionMask(uintptr_t pawn) {
    const uintptr_t mgr = ResolveInventoryManager(pawn);
    if (!mgr || !PauseRttProbe::SafeRange(mgr + kMgrNativeMaskOff, 4)) return 0;
    return *reinterpret_cast<const uint32_t*>(mgr + kMgrNativeMaskOff);
}

inline const char* AvailabilityBlockReason(uintptr_t pawn) {
    if (!pawn) return "no_pawn";
    const uintptr_t gs = GameSystemInstance();
    if (!gs || !PauseRttProbe::SafeRange(gs + kCatalogCountOff, 8)) return "availability_gamesystem";
    if (!LockerVrLoadoutIdentity::GameSystemObjectIdentityOk(gs)) return "availability_gamesystem_identity";
    // Owned inventory snapshot already validated pawn+0x3D0 manager via class chain (same thread as loadout poll).
    if (NativeOwnedInventory::SnapshotValid() && NativeOwnedInventory::SnapshotPawn() == pawn) return nullptr;
    if (!ResolveInventoryManager(pawn)) return "availability_manager";
    return nullptr;
}

inline bool AvailabilitySourcesReady(uintptr_t pawn) { return AvailabilityBlockReason(pawn) == nullptr; }

inline uint32_t AvailableOrdinaryMask(uintptr_t pawn) {
    if (!AvailabilitySourcesReady(pawn)) return 0;
    const uintptr_t gs = GameSystemInstance();
    if (!gs || !PauseRttProbe::SafeRange(gs + kCatalogCountOff, 8)) return 0;
    const uint32_t nativeMask = ReadNativeProgressionMask(pawn);
    return LockerVrLoadoutPolicy::AvailableOrdinaryMaskFromNative(nativeMask, RowAtCallback);
}

inline const void* FStringPtrAtCatalogRowSecondary(unsigned index) {
    const uintptr_t gs = GameSystemInstance();
    if (!gs || !PauseRttProbe::SafeRange(gs + kCatalogCountOff, 4)) return nullptr;
    const int count = *reinterpret_cast<const int*>(gs + kCatalogCountOff);
    if (index >= static_cast<unsigned>(count)) return nullptr;
    const uintptr_t data = PauseRttProbe::Pointer(gs + kCatalogDataOff);
    if (!data) return nullptr;
    const uintptr_t row = data + static_cast<uintptr_t>(index) * kCatalogStride;
    if (!PauseRttProbe::SafeRange(row + 0x10, 4)) return nullptr;
    if (*reinterpret_cast<const int*>(row + 0x10) == 0)
        return reinterpret_cast<const void*>(kDefaultEmptyFString);
    if (!PauseRttProbe::SafeRange(row + 0xC, 8)) return nullptr;
    return reinterpret_cast<const void*>(PauseRttProbe::Pointer(row + 0xC));
}

} // namespace LockerVrLoadoutRuntime
