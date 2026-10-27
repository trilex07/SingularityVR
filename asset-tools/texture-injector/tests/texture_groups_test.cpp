// Unit test for texture group gating (no D3D, no game).
// Build: cl /nologo /EHsc /W4 /O2 /std:c++17 texture_groups_test.cpp
#include <cstdio>
#include <cstdlib>

#include "../texture_groups.h"

static int g_fail = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", msg); ++g_fail; }
}

using texgroups::GCount;
using texgroups::GEnemies;
using texgroups::GEnv;
using texgroups::GFx;
using texgroups::GGlyphs;
using texgroups::GHands;
using texgroups::GWeapons;

int main() {
    Check(texgroups::GroupForName("inview_glove_d") == GHands, "glove hands");
    Check(texgroups::GroupForName("handgun_d") == GHands, "pistol hands");
    Check(texgroups::GroupForName("laser_n") == GHands, "laser hands");
    Check(texgroups::GroupForName("AR_E99Rifle_alt_d") == GWeapons, "E99 weapons");
    Check(texgroups::GroupForName("ar_gatling_gun_d") == GWeapons, "gatling weapons");
    Check(texgroups::GroupForName("AR_shotgun_d") == GWeapons, "shotgun weapons");
    Check(texgroups::GroupForName("ar_sniper_rifle_n") == GWeapons, "sniper weapons");
    Check(texgroups::GroupForName("elite_beam_n") == GWeapons, "railgun weapons");
    Check(texgroups::GroupForName("TMD_new_n") == GWeapons, "TMD weapons");
    Check(texgroups::GroupForName("revert_d2") == GEnemies, "revert enemies");
    Check(texgroups::GroupForName("phasetick_d") == GEnemies, "phasetick enemies");
    Check(texgroups::GroupForName("Spetsnaz_MP_Body_d") == GEnemies, "spetsnaz enemies");
    Check(texgroups::GroupForName("door_blastdoorN_d") == GEnv, "door env");
    Check(texgroups::GroupForName("env_shutter_d") == GEnv, "shutter env");
    Check(texgroups::GroupForName("env_00adab7c") == GEnv, "captured env");
    Check(texgroups::GroupForName("360_A") == GGlyphs, "glyph");
    Check(texgroups::GroupForName("bsplat3_d") == GFx, "blood fx");
    Check(texgroups::GroupForName("muzzleflashCenter") == GFx, "muzzle fx");
    Check(texgroups::GroupForName("smoke_dark_subUV") == GFx, "smoke fx");
    Check(texgroups::GroupForName("tracer2") == GFx, "tracer fx");
    Check(texgroups::GroupForName("fireball_base_intense_02") == GFx, "fire fx");
    Check(texgroups::GroupForName("t_decal_metal_d_v4") == GFx, "decal fx");
    Check(texgroups::GroupForName("something_new_thing") == GFx, "unknown falls to fx");
    Check(texgroups::GroupForName(nullptr) == GFx, "null falls to fx");
    Check(texgroups::GroupForName("") == GFx, "empty falls to fx");

    bool on[GCount] = {true, true, true, true, true, true};
    Check(texgroups::GroupAllows(on, GHands), "all-on allows");
    on[GEnemies] = false;
    Check(!texgroups::GroupAllows(on, GEnemies), "disabled group blocks");
    Check(texgroups::GroupAllows(on, GHands), "other groups unaffected");
    Check(texgroups::GroupAllows(on, -1), "bad index fails open");
    Check(texgroups::GroupAllows(on, 99), "bad index fails open");
    Check(texgroups::GroupIndex("hands") == GHands, "index hands");
    Check(texgroups::GroupIndex("fx") == GFx, "index fx");
    Check(texgroups::GroupIndex("nope") == -1, "index unknown fails open");
    Check(texgroups::GroupIndex(nullptr) == -1, "index null fails open");
    Check(!std::strcmp(texgroups::QualitySuffix("2x"), ""), "2x has no suffix");
    Check(!std::strcmp(texgroups::QualitySuffix("4K"), "-4K"), "4K suffix");
    Check(!std::strcmp(texgroups::QualitySuffix("8K"), "-8K"), "8K suffix");
    Check(!std::strcmp(texgroups::QualitySuffix("16K"), ""), "16K banned to 2x");
    Check(!std::strcmp(texgroups::QualitySuffix(nullptr), ""), "null to 2x");

    if (g_fail) { std::fprintf(stderr, "texture-groups tests: %d FAILED\n", g_fail); return 1; }
    std::puts("PASS: texture group mapping + gating");
    return 0;
}
