#include "../locker_loadout_icons.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <cstdio>

int main(int argc, char** argv) {
    if (argc != 2) { std::puts("usage: test-locker-loadout-icons <atlas.json>"); return 2; }
    std::ifstream f(argv[1], std::ios::binary);
    assert(f.good());
    const std::string json((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    hud_atlas::Sheet sheet{};
    assert(hud_atlas::Parse(json.c_str(), sheet));
    assert(sheet.width == 512 && sheet.height == 256);
    const int meshes[] = {5799, 11387, 8297, 11389};
    std::set<std::string> ids;
    std::set<std::pair<int, int>> origins;
    for (int mesh : meshes) {
        const auto* r = locker_loadout_icons::FindValid(sheet, mesh);
        assert(r && r->w == 64 && r->h == 64);
        assert(r->x >= 0 && r->y >= 0 && r->x + r->w <= sheet.width && r->y + r->h <= sheet.height);
        const float u0 = float(r->x) / sheet.width, v0 = float(r->y) / sheet.height;
        const float u1 = float(r->x + r->w) / sheet.width, v1 = float(r->y + r->h) / sheet.height;
        assert(u0 >= 0 && v0 >= 0 && u1 <= 1 && v1 <= 1 && u0 < u1 && v0 < v1);
        ids.insert(r->id); origins.insert({r->x, r->y});
        std::printf("mesh=%d sprite=%s uv=(%.3f,%.3f)-(%.3f,%.3f)\n", mesh, r->id.c_str(), u0, v0, u1, v1);
    }
    assert(ids.size() == 4 && origins.size() == 4);
    assert(locker_loadout_icons::FindValid(sheet, 0) == nullptr);
    hud_atlas::Sheet broken = sheet;
    for (auto& r : broken.rects) if (r.id == "gun-centurion") r.x = 500;
    assert(locker_loadout_icons::FindValid(broken, 5799) == nullptr);
    std::puts("locker loadout atlas mapping PASS");
    return 0;
}
