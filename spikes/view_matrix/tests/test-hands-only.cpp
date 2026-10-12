#include "../glove_isolate.h"
#include <cstdio>

static int g_fails = 0;

static void Expect(bool cond, const char* name) {
    if (cond) {
        std::printf("PASS %s\n", name);
        return;
    }
    std::printf("FAIL %s\n", name);
    ++g_fails;
}

int main() {
    Expect(gloveisolate::HandsOnly(0, 2), "isolate 2 with arms hidden is hands-only");
    Expect(gloveisolate::HandsOnly(0, 3), "isolate 3 is still hands-only");
    Expect(!gloveisolate::UsesPaletteStump(0, 2), "mode 2 does not palette-stump");
    Expect(gloveisolate::UsesPaletteStump(0, 3), "mode 3 keeps legacy stump");
    Expect(!gloveisolate::HandsOnly(1, 2), "showing controller arms is not hands-only");
    Expect(!gloveisolate::HandsOnly(0, 0), "isolation off is not hands-only");
    Expect(!gloveisolate::HandsOnly(0, 1), "census mode does not hide the arm");

    Expect(gloveisolate::IsArmFleshSection(7444, 13206, 7704),
           "the long TMD forearm section is the hidden arm");
    Expect(gloveisolate::IsArmFleshSection(4818, 13206, 7704),
           "the gauntlet forearm section is the same hidden draw");
    Expect(!gloveisolate::IsArmFleshSection(7444, 0, 4402),
           "the glove section is not the hidden arm");
    Expect(!gloveisolate::IsArmFleshSection(3314, 0, 4830),
           "the pistol glove section is not the hidden arm");
    Expect(gloveisolate::IsStrayFingerSection(390, 14490, 674),
           "the detached sleeve stays a separate hidden draw");

    gloveisolate::Mat4 stump;
    const float wrist[3] = { 10.0f, 0.0f, 0.0f };
    gloveisolate::CollapseDelta(wrist, stump, 0.12f);
    Expect(stump.m[0][0] > 0.0f && stump.m[0][0] < 1.0f,
           "legacy mode 3 stump remains a scale toward the wrist");

    Expect(gloveisolate::NoTmdGlovePaletteCovers(6), "left clavicle is in the glove palette");
    Expect(gloveisolate::NoTmdGlovePaletteCovers(7), "left shoulder is in the glove palette");
    Expect(gloveisolate::NoTmdGlovePaletteCovers(8), "left elbow is in the glove palette");
    Expect(gloveisolate::NoTmdGlovePaletteCovers(9), "left wrist is in the glove palette");
    Expect(gloveisolate::NoTmdGlovePaletteCovers(27), "left forearm is in the glove palette");
    Expect(gloveisolate::NoTmdGlovePaletteCovers(41), "left sleeve_fix is in the glove palette");
    Expect(gloveisolate::NoTmdGlovePaletteCovers(46), "left forearm2 is in the glove palette");
    Expect(gloveisolate::NoTmdGlovePaletteCovers(47), "left bicep is in the glove palette");
    Expect(gloveisolate::kNoTmdGlovePaletteBoneCount == 45u,
           "pistol glove palette slot count");
    return g_fails == 0 ? 0 : 1;
}
