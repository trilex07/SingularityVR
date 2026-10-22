#pragma once
// Offline policy for WeaponLocker ordinary-firearm virtualization.
// Witness: inspect_locker_runtime_contract.py — Start uses controller slots 0/1;
// PopMenu writes SetWeaponSlotName(i) for every equipped[i] (i>=2 clobbers specials);
// equip has no bounds check on script slotNum.
//
// This header is pure logic only. Runtime hooks are described in
// docs/locker-virtual-contract-sketch.md. Default OFF at runtime.

namespace lockervirtual {

constexpr int kMaxOrdinaryMeshes = 4;
constexpr int kNativeOrdinaryControllerSlots = 2; // Start witness: GetWeaponSlotName(0,1)
constexpr int kReservedControllerSlotStart = 2;   // heavy / grenade / special — never overwrite

inline bool OrdinaryMesh(int mesh) {
    switch (mesh) {
    case 5799:
    case 11387:
    case 8297:
    case 11389:
        return true;
    default:
        return false;
    }
}

inline unsigned MeshesFromOwnedMask(unsigned mask, int* out, unsigned cap) {
    static const int kOrdinary[] = {5799, 11387, 8297, 11389};
    unsigned n = 0;
    for (unsigned i = 0; i < kMaxOrdinaryMeshes && n < cap; ++i) {
        if ((mask & (1u << i)) == 0) continue;
        if (out) out[n] = kOrdinary[i];
        ++n;
    }
    return n;
}

struct PopMenuWriteDecision {
    bool allow = false;
    int controllerSlot = -1; // SetWeaponSlotName index when allow
};

// Native PopMenu loops equipped index i and calls SetWeaponSlotName(i, ...).
// Virtual layer must refuse i >= kReservedControllerSlotStart regardless of TArray size.
inline PopMenuWriteDecision DecidePopMenuControllerWrite(int equippedIndex, int equippedCount) {
    PopMenuWriteDecision d{};
    if (equippedIndex < 0 || equippedIndex >= equippedCount) return d;
    if (equippedIndex >= kReservedControllerSlotStart) return d;
    if (equippedIndex >= kNativeOrdinaryControllerSlots) return d;
    d.allow = true;
    d.controllerSlot = equippedIndex;
    return d;
}

struct EquipDecision {
    bool allow = false;
    int equippedIndex = -1;
};

// Native equip uses script slotNum as TArray index without bounds check.
inline EquipDecision DecideEquipIndex(int slotNum, int equippedCount) {
    EquipDecision d{};
    if (slotNum < 0 || slotNum >= equippedCount) return d;
    if (slotNum >= kMaxOrdinaryMeshes) return d;
    d.allow = true;
    d.equippedIndex = slotNum;
    return d;
}

// Phase-1 virtual load: UI may list up to four ordinary weapons; only the first two
// controller slots participate in Start's native producer. Extra entries are for
// GFx selection + LoadEquippedWeapons grant path (phase 3), not controller slot 2/3.
struct VirtualLoadView {
    int uiOrdinaryCount = 0;
    int uiMeshes[kMaxOrdinaryMeshes]{};
    int controllerOrdinaryCount = 0; // min(uiOrdinaryCount, kNativeOrdinaryControllerSlots)
};

inline VirtualLoadView PlanVirtualLoad(unsigned ownedMask) {
    VirtualLoadView v{};
    v.uiOrdinaryCount = (int)MeshesFromOwnedMask(ownedMask, v.uiMeshes, kMaxOrdinaryMeshes);
    if (v.uiOrdinaryCount > kNativeOrdinaryControllerSlots)
        v.controllerOrdinaryCount = kNativeOrdinaryControllerSlots;
    else
        v.controllerOrdinaryCount = v.uiOrdinaryCount;
    return v;
}

} // namespace lockervirtual
