#pragma once
// WeaponLocker icon identity is independent of the lazily loaded weapon-wheel catalog.
#include "weapon_wheel_atlas.h"

namespace locker_loadout_icons {
inline const char* SpriteForMesh(int mesh) {
    switch (mesh) {
    case 5799: return "gun-centurion";
    case 11387: return "gun-valkyrie";
    case 8297: return "gun-spikeshot"; // Volk's atlas sprite
    case 11389: return "gun-kasimov";
    default: return nullptr;
    }
}

inline const hud_atlas::Rect* FindValid(const hud_atlas::Sheet& sheet, int mesh) {
    const auto* r = hud_atlas::Find(sheet, SpriteForMesh(mesh));
    if (!r || sheet.width <= 0 || sheet.height <= 0 || r->x < 0 || r->y < 0 ||
        r->w <= 0 || r->h <= 0 || r->x > sheet.width - r->w || r->y > sheet.height - r->h)
        return nullptr;
    return r;
}
} // namespace locker_loadout_icons
