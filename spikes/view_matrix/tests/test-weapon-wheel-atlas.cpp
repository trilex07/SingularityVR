#include "../weapon_wheel_atlas.h"
#include <cstdio>

static int g_fails = 0;
static void Expect(bool cond, const char* name) {
    if (cond) { std::printf("PASS %s\n", name); return; }
    std::printf("FAIL %s\n", name);
    ++g_fails;
}

static const char kSample[] =
    "{\"name\":\"katorga-hud\",\"width\":512,\"height\":256,"
    "\"png_sha256\":\"abc123\","
    "\"rects\":["
    "{\"id\":\"mag-closed\",\"x\":192,\"y\":0,\"w\":64,\"h\":64},"
    "{\"id\":\"rivet\",\"x\":448,\"y\":0,\"w\":64,\"h\":64}]}";

int main() {
    hud_atlas::Sheet sheet;
    Expect(hud_atlas::Parse(kSample, sheet), "atlas parses");
    Expect(sheet.width == 512 && sheet.height == 256, "sheet dims");
    Expect(sheet.pngSha256 == "abc123", "png hash captured");
    Expect(sheet.rects.size() == 2, "two rects");
    const hud_atlas::Rect* r = hud_atlas::Find(sheet, "mag-closed");
    Expect(r && r->x == 192 && r->y == 0 && r->w == 64 && r->h == 64, "rect lookup");
    Expect(hud_atlas::Find(sheet, "nope") == nullptr, "missing id refused");
    hud_atlas::Sheet bad;
    Expect(!hud_atlas::Parse("{\"nope\":[]}", bad), "garbage refused");
    Expect(!hud_atlas::Parse(nullptr, bad), "null refused");
    return g_fails == 0 ? 0 : 1;
}
