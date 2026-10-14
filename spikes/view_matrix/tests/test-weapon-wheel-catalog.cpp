#include "../weapon_wheel_catalog_load.h"
#include "../weapon_wheel_direct_select.h"
#include <cstdio>

static int g_fails = 0;
static void Expect(bool cond, const char* name) {
    if (cond) { std::printf("PASS %s\n", name); return; }
    std::printf("FAIL %s\n", name);
    ++g_fails;
}

static const char kSample[] =
    "{\"sectors\":["
    "{\"index\":0,\"label_pl\":\"Brak\",\"gun_verts\":0},"
    "{\"index\":1,\"label_pl\":\"Centurion\",\"gun_verts\":5799,\"atlas_icon\":\"mag-closed\"},"    "{\"index\":2,\"label_pl\":\"AR\",\"gun_verts\":11387},"
    "{\"index\":3,\"label_pl\":\"Shotgun\",\"gun_verts\":8297},"
    "{\"index\":4,\"label_pl\":\"Snajper\",\"gun_verts\":11389},"
    "{\"index\":5,\"label_pl\":\"-\",\"gun_verts\":0},"
    "{\"index\":6,\"label_pl\":\"-\",\"gun_verts\":0},"
    "{\"index\":7,\"label_pl\":\"-\",\"gun_verts\":0}],"
    "\"cycle_order\":[5799,11387,8297,11389],"
    "\"cycle_order_verified\":false}";

int main() {
    wheel_catalog::Catalog cat;
    Expect(wheel_catalog::Parse(kSample, cat), "catalog parses");
    Expect(cat.sectors.size() == 8, "eight sectors");
    Expect(cat.sectors[1].verts == 5799, "sector1 verts");
    Expect(cat.sectors[1].atlas == "mag-closed", "sector1 icon");
    Expect(cat.cycle.size() == 4, "four cycle entries");

    wheel_catalog::Catalog bad;
    Expect(!wheel_catalog::Parse("{\"nope\":[]}", bad), "garbage refused");
    Expect(!wheel_catalog::Parse(nullptr, bad), "null refused");

    static weaponwheel_direct::CatalogSector secs[8];
    for (size_t i = 0; i < cat.sectors.size() && i < 8; ++i) {
        secs[i].sector = cat.sectors[i].index;
        secs[i].gunVerts = cat.sectors[i].verts;
        secs[i].classHint = nullptr;
        secs[i].commonTextKey = nullptr;
        secs[i].labelPl = nullptr;
        secs[i].atlasIcon = nullptr;
    }
    static long cyc[4];
    for (size_t i = 0; i < cat.cycle.size() && i < 4; ++i) cyc[i] = cat.cycle[i];
    const weaponwheel_direct::Catalog view{ secs, 8, cyc, 4 };
    Expect(weaponwheel_direct::TargetVertsForSector(view, 1) == 5799, "sector1 target");
    Expect(weaponwheel_direct::TargetVertsForSector(view, 0) == 0, "unused sector target");
    Expect(weaponwheel_direct::TargetVertsForSector(view, 9) == 0, "out-of-range target");
    Expect(weaponwheel_direct::ShortestCycleSteps(5799, 5799, cyc, 4) == 0, "same weapon zero steps");
    Expect(weaponwheel_direct::ShortestCycleSteps(5799, 8297, cyc, 4) == 2, "forward distance");
    Expect(weaponwheel_direct::ShortestCycleSteps(8297, 5799, cyc, 4) == 2, "wrap distance");
    Expect(weaponwheel_direct::ShortestCycleSteps(1, 5799, cyc, 4) == -1, "unknown current refused");
    Expect(weaponwheel_direct::ShortestCycleSteps(5799, 1, cyc, 4) == -1, "unknown target refused");
    return g_fails == 0 ? 0 : 1;
}
