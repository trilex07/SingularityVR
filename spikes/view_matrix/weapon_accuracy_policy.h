#pragma once
#include "weapon_muzzle_geometry.h"

namespace weaponaccuracy {
// The vertex signature alone can lag a weapon switch. Require a fresh witness
// produced by the same rendered weapon before touching native AddSpread output.
inline int EffectivePercent(int vertices, weaponreload::Kind kind, bool currentWitness,
                            bool support, int twoHandPercent, int valkyriePercent,
                            int volkPercent) {
    if (!currentWitness) return 100;
    int profile = 100;
    if (vertices == 11387 && kind == weaponreload::Kind::Valkyrie)
        profile = valkyriePercent;
    else if (vertices == 8297 && kind == weaponreload::Kind::Volk)
        profile = volkPercent;
    else return 100;
    if (profile < 0 || profile > 100 || twoHandPercent < 0 || twoHandPercent > 100)
        return 100;
    // One reduction of the native angular delta, never two successive writes.
    return support ? (profile * twoHandPercent + 50) / 100 : profile;
}
}
