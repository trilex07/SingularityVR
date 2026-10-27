#pragma once
// Proposal for SingularityVR-recovery — copy into spikes/view_matrix/ when integrating.
// SIDE does not compile this into the mod DLL.

#include <cstddef>

namespace weaponwheel_direct {

struct CatalogSector {
    int sector;              // 0..slots-1, 0 = current / holster slot display only
    long gunVerts;           // g_gunVerts latch; 0 = unused sector
    const char* classHint;   // e.g. "ARPistol" — log only until verified
    const char* commonTextKey;
    const char* labelPl;
    const char* atlasIcon;   // id in katorga atlas.json
};

struct Catalog {
    const CatalogSector* sectors;
    int sectorCount;
    const long* cycleOrder;  // native CycleWeapon order, verified in-game
    int cycleCount;
};

inline int ShortestCycleSteps(long currentVerts, long targetVerts,
                              const long* cycleOrder, int cycleCount) {
    if (!cycleOrder || cycleCount <= 0 || currentVerts == targetVerts) return 0;
    int i = -1, j = -1;
    for (int k = 0; k < cycleCount; ++k) {
        if (cycleOrder[k] == currentVerts) i = k;
        if (cycleOrder[k] == targetVerts) j = k;
    }
    if (i < 0 || j < 0) return -1;
    int d = j - i;
    if (d < 0) d += cycleCount;
    return d;
}

inline long TargetVertsForSector(const Catalog& cat, int selection) {
    if (!cat.sectors || selection < 0 || selection >= cat.sectorCount) return 0;
    return cat.sectors[selection].gunVerts;
}

// After Commit: set g_weaponWheelCommit pulses OR run stepped pulse loop with g_gunVerts check.
// Implementation lives in d3d9.cpp near existing ~9988 block.

} // namespace weaponwheel_direct
