#pragma once

#include <cstring>

// Texture group gating for the override injector (Variant B).
//
// Groups let the player pick, per category, which replacements load
// ([TextureGroups] in texture_override.ini, read once at startup; a missing
// key defaults to ON). The hub panel (main agent) writes these keys.
// No D3D, no I/O, no locks in here: the host copies the six flags once and
// calls GroupAllows() on the already-matched target (cached per texture).

namespace texgroups {

enum Group : int {
    GHands = 0,    // first-person gloves, pistol, laser sight
    GWeapons,      // E99, shotgun, gatling, sniper, railgun, TMD device
    GEnemies,      // Revert, Phase Tick, Spetsnaz
    GEnv,          // doors, shutters, captured surfaces
    GGlyphs,       // UI button glyphs (360_*)
    GFx,           // blood, muzzle, smoke, fire, sparks, tracers, decals
    GCount
};

inline const char* GroupName(int g) {
    switch (g) {
    case GHands: return "hands";
    case GWeapons: return "weapons";
    case GEnemies: return "enemies";
    case GEnv: return "env";
    case GGlyphs: return "glyphs";
    case GFx: return "fx";
    default: return "unknown";
    }
}

// Prefix rules mirror the pipeline's texture_group() (asset-tools/textures).
// Unknown names fall into GFx (visible effects) rather than failing closed:
// a wrong-group texture still substitutes, it is just toggled with effects.
inline int GroupForName(const char* name) {
    if (!name || !*name) return GFx;
    // hands: first-person gloves, pistol, laser
    if (!strncmp(name, "inview_glove_", 13)) return GHands;
    if (!strncmp(name, "handgun_", 8)) return GHands;
    if (!strncmp(name, "laser_", 6)) return GHands;
    // enemies
    if (!strncmp(name, "revert_", 7)) return GEnemies;
    if (!strncmp(name, "phasetick_", 10)) return GEnemies;
    if (!strncmp(name, "Spetsnaz_", 9)) return GEnemies;
    // env: doors, shutters, captured surfaces
    if (!strncmp(name, "door_", 5)) return GEnv;
    if (!strncmp(name, "env_", 4)) return GEnv;
    // glyphs
    if (!strncmp(name, "360_", 4)) return GGlyphs;
    // weapons
    if (!strncmp(name, "AR_", 3)) return GWeapons;
    if (!strncmp(name, "ar_", 3)) return GWeapons;
    if (!strncmp(name, "elite_beam_", 11)) return GWeapons;
    if (!strncmp(name, "TMD_", 4)) return GWeapons;
    if (!strncmp(name, "matball", 7)) return GWeapons;
    if (!strncmp(name, "DETAIL", 6)) return GWeapons;
    if (!strncmp(name, "AmmoGauge", 9)) return GWeapons;
    if (!strncmp(name, "scope_", 6)) return GWeapons;
    if (!strncmp(name, "reddot", 6)) return GWeapons;
    if (!strncmp(name, "grey", 4)) return GWeapons;
    return GFx;
}

// Quality tiers: 2x is the full set in <texturesdir>\; 4K/8K live in
// <texturesdir>-4K\ and <texturesdir>-8K\. Unknown values fall back to 2x.
inline const char* QualitySuffix(const char* quality) {
    if (!quality) return "";
    if (!std::strcmp(quality, "4K")) return "-4K";
    if (!std::strcmp(quality, "8K")) return "-8K";
    return "";
}

inline int GroupIndex(const char* groupName) {
    if (!groupName) return -1;
    for (int g = 0; g < GCount; ++g)
        if (!std::strcmp(groupName, GroupName(g))) return g;
    return -1;  // unknown -> GroupAllows fails open
}

inline bool GroupAllows(const bool enabled[GCount], int group) {
    if (group < 0 || group >= GCount) return true;  // fail open, never hide
    return enabled[group];
}

}  // namespace texgroups

