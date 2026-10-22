#pragma once
// Pure policy: native manager +0x3E4 bits + FE2500 catalog row eligibility → VR ordinary mask.
#include <cstdint>

namespace LockerVrLoadoutPolicy {

constexpr unsigned kCatalogIndexPistol = 0u;
constexpr unsigned kCatalogIndexMg = 1u;
constexpr unsigned kCatalogIndexShotgun = 2u;
constexpr unsigned kCatalogIndexSniper = 5u;

struct CatalogRowView {
    uint8_t row24 = 0xFFu;
    uint32_t row4c = 0xFFFFFFFFu;
    bool readable = false;
};

inline bool RowEligible(const CatalogRowView& row) {
    return row.readable && row.row24 == 0 && ((row.row4c & 1u) == 0u);
}

inline uint32_t NativeBitForCatalogIndex(unsigned index) { return 1u << index; }

inline uint32_t VrBitForNativeBit(uint32_t nativeBit) {
    switch (nativeBit) {
    case 1u:
        return 1u;
    case 2u:
        return 2u;
    case 4u:
        return 4u;
    case 0x20u:
        return 8u;
    default:
        return 0u;
    }
}

inline uint32_t VrBitForCatalogIndex(unsigned index) {
    return VrBitForNativeBit(NativeBitForCatalogIndex(index));
}

struct NativeIndexMap {
    unsigned index;
    uint32_t nativeBit;
};

inline constexpr NativeIndexMap kOrdinaryMaps[] = {
    {kCatalogIndexPistol, 1u},
    {kCatalogIndexMg, 2u},
    {kCatalogIndexShotgun, 4u},
    {kCatalogIndexSniper, 0x20u},
};

inline uint32_t AvailableOrdinaryMaskFromNative(
    uint32_t nativeMask, bool (*rowAt)(unsigned index, CatalogRowView* out)) {
    if (!rowAt) return 0;
    uint32_t vr = 0;
    for (const auto& m : kOrdinaryMaps) {
        if ((nativeMask & m.nativeBit) == 0) continue;
        CatalogRowView row{};
        if (!rowAt(m.index, &row) || !RowEligible(row)) continue;
        vr |= VrBitForNativeBit(m.nativeBit);
    }
    return vr;
}

inline uint32_t PendingMask(uint32_t available, uint32_t owned) { return available & ~owned; }

inline uint32_t RevalidateQueuedMask(uint32_t queued, uint32_t available, uint32_t owned) {
    return queued & available & ~owned;
}

inline const char* WeaponLogNameForVrBit(uint32_t vrBit) {
    switch (vrBit) {
    case 1u:
        return "ARPistol";
    case 2u:
        return "ARMachineGun";
    case 4u:
        return "ARShotgun";
    case 8u:
        return "ARSniperRifle";
    default:
        return nullptr;
    }
}

inline unsigned CatalogIndexForVrBit(uint32_t vrBit) {
    switch (vrBit) {
    case 1u:
        return kCatalogIndexPistol;
    case 2u:
        return kCatalogIndexMg;
    case 4u:
        return kCatalogIndexShotgun;
    case 8u:
        return kCatalogIndexSniper;
    default:
        return ~0u;
    }
}

} // namespace LockerVrLoadoutPolicy
