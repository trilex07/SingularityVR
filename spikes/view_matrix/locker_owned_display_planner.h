#pragma once
#include "inventory_retention_policy.h"
#include <cstdint>

namespace lockerowned {

constexpr int kMaxExtraTiles = 2;
constexpr int kOrdinaryMeshes[4] = {5799, 11387, 8297, 11389};

inline bool OrdinaryMesh(int mesh) {
    return inventoryretention::WeaponBit(mesh) != 0;
}

inline unsigned OwnedBitForMesh(int mesh) { return inventoryretention::WeaponBit(mesh); }

struct ExtraTilePlan {
    int count = 0;
    int meshes[kMaxExtraTiles]{0, 0};
};

// Retained ordinary weapons in owned mask that are not shown in the native GFx pair.
inline ExtraTilePlan PlanExtraTiles(unsigned ownedMask, int nativeMesh0, int nativeMesh1) {
    ExtraTilePlan plan{};
    const unsigned nativeBits = OwnedBitForMesh(nativeMesh0) | OwnedBitForMesh(nativeMesh1);
    const unsigned extraMask = ownedMask & ~nativeBits;
    for (unsigned i = 0; i < 4 && plan.count < kMaxExtraTiles; ++i) {
        if ((extraMask & (1u << i)) == 0) continue;
        plan.meshes[plan.count++] = kOrdinaryMeshes[i];
    }
    return plan;
}

inline bool MeshAllowedInPlan(int mesh, unsigned ownedMask) {
    if (!OrdinaryMesh(mesh)) return false;
    const unsigned bit = OwnedBitForMesh(mesh);
    return bit != 0 && (ownedMask & bit) != 0;
}

} // namespace lockerowned
