#include "../plates_atlas.h"
#include <cstdio>

static int g_fails = 0;
static void Expect(bool cond, const char* name) {
    if (cond) { std::printf("PASS %s\n", name); return; }
    std::printf("FAIL %s\n", name);
    ++g_fails;
}

// Same shape as the installed plates.json (ids verified against it).
static const char kSample[] =
    "{\"name\":\"katorga-plates\",\"width\":512,\"height\":256,"
    "\"png_sha256\":\"010e2853\","
    "\"rects\":["
    "{\"id\":\"plate-health-bg\",\"x\":0,\"y\":0,\"w\":384,\"h\":64},"
    "{\"id\":\"seg-full\",\"x\":384,\"y\":0,\"w\":64,\"h\":114},"
    "{\"id\":\"seg-empty\",\"x\":448,\"y\":0,\"w\":64,\"h\":114}]}";

int main() {
    plates_atlas::Sheet sheet;
    Expect(plates_atlas::Parse(kSample, sheet), "plates parse");
    Expect(sheet.width == 512 && sheet.height == 256, "plates dims");
    Expect(sheet.rects.size() == 3, "three rects");
    const plates_atlas::Rect* bg = plates_atlas::Find(sheet, "plate-health-bg");
    Expect(bg && bg->x == 0 && bg->y == 0 && bg->w == 384 && bg->h == 64, "bg rect");
    Expect(plates_atlas::Find(sheet, "seg-full") != nullptr, "full rect");
    Expect(plates_atlas::Find(sheet, "seg-empty") != nullptr, "empty rect");
    Expect(plates_atlas::Find(sheet, "nope") == nullptr, "missing id refused");
    // Every rect must sit inside the sheet (stamping never reads outside).
    bool inside = true;
    for (size_t i = 0; i < sheet.rects.size(); ++i) {
        const plates_atlas::Rect& r = sheet.rects[i];
        if (r.x < 0 || r.y < 0 || r.x + r.w > sheet.width || r.y + r.h > sheet.height) inside = false;
    }
    Expect(inside, "rects inside sheet");
    plates_atlas::Sheet bad;
    Expect(!plates_atlas::Parse("{\"nope\":[]}", bad), "garbage refused");
    Expect(!plates_atlas::Parse(nullptr, bad), "null refused");
    if (g_fails == 0) std::puts("health bar: plates atlas L1");
    return g_fails == 0 ? 0 : 1;
}
