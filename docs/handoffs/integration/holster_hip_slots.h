#pragma once

// Hip slot helpers for MAIN (mirror of weapon holster frame).
// Copy next to weapon_holster.h in recovery.

#include "weapon_holster.h"

namespace holsterslots {

inline weaponholster::Config MedkitConfigFromWeapon(const weaponholster::Config& weapon,
                                                    float medkitRightUU) {
    weaponholster::Config c = weapon;
    c.right = medkitRightUU;
    return c;
}

inline weaponholster::V3 MedkitHipPoint(const weaponholster::Config& weaponCfg,
                                        float medkitRightUU, float bodyYawRad) {
    return weaponholster::HipPoint(MedkitConfigFromWeapon(weaponCfg, medkitRightUU), bodyYawRad);
}

inline bool NearMedkitHip(weaponholster::V3 hand, const weaponholster::Config& weaponCfg,
                          float medkitRightUU, float medkitRadiusUU, float bodyYawRad) {
    const weaponholster::V3 hip =
        MedkitHipPoint(weaponCfg, medkitRightUU, bodyYawRad);
    weaponholster::Config probe{};
    probe.radius = medkitRadiusUU;
    return weaponholster::NearHip(hand, hip, probe);
}

} // namespace holsterslots
