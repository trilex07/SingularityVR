#pragma once
#include "weapon_muzzle_geometry.h"

namespace weaponaccuracy {
struct Settings {
    int globalTwoHand=100;
    int valkyrie=100, volk=100;
    int valkyrieTwoHand=100, volkTwoHand=100;
};
// The vertex signature alone can lag a weapon switch. Require a fresh witness
// produced by the same rendered weapon before touching native AddSpread output.
inline int EffectivePercent(int vertices, weaponreload::Kind kind, bool currentWitness,
                            bool support, const Settings& settings) {
    const int global=settings.globalTwoHand;
    if (global < 0 || global > 100) return 100;
    const int legacy=support?global:100;
    if (!currentWitness) return legacy;
    int profile=100, scopedTwoHand=100;
    if (vertices == 11387 && kind == weaponreload::Kind::Valkyrie) {
        profile=settings.valkyrie;scopedTwoHand=settings.valkyrieTwoHand;
    } else if (vertices == 8297 && kind == weaponreload::Kind::Volk) {
        profile=settings.volk;scopedTwoHand=settings.volkTwoHand;
    } else return legacy;
    if (profile < 0 || profile > 100 || scopedTwoHand < 0 || scopedTwoHand > 100)
        return legacy;
    // Existing global two-hand behavior remains a factor. The weapon-scoped
    // factor applies only after a matching witness; AddSpread is written once.
    return (profile * legacy * (support?scopedTwoHand:100) + 5000) / 10000;
}
}
