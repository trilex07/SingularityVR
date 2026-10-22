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
    Expect(gloveisolate::GlovesUntilWeapon(1, 2, false), "arms setting still uses gloves before a weapon");
    Expect(!gloveisolate::GlovesUntilWeapon(1, 2, true), "a held weapon keeps the controller-arm path");
    Expect(gloveisolate::GlovesUntilWeapon(0, 2, false), "gloves-only stays gloves without a weapon");
    Expect(!gloveisolate::GlovesUntilWeapon(1, 0, false), "isolation off does not switch paths");
    Expect(!gloveisolate::HandsOnly(0, 0), "isolation off is not hands-only");
    Expect(!gloveisolate::HandsOnly(0, 1), "census mode does not hide the arm");

    Expect(gloveisolate::IsArmFleshSection(7444, 13206, 7704),
           "the mixed TMD hardware/forearm section is identified exactly");
    Expect(gloveisolate::IsArmFleshSection(4818, 13206, 7704),
           "the gauntlet forearm section is the same hidden draw");
    Expect(!gloveisolate::IsArmFleshSection(7444, 0, 4402),
           "the glove section is not the hidden arm");
    Expect(!gloveisolate::IsArmFleshSection(3314, 0, 4830),
           "the pistol glove section is not the hidden arm");
    Expect(gloveisolate::IsStrayFingerSection(390, 14490, 674),
           "the detached sleeve stays a separate hidden draw");
    Expect(gloveisolate::TmdHardwareFaceCount()==6527,
           "TMD hardware remains visible without 1177 arm faces");
    Expect(gloveisolate::TmdHardwareCutApplies(2,7444,13206,7704)&&
           gloveisolate::TmdHardwareCutApplies(3,4818,13206,7704)&&
           !gloveisolate::TmdHardwareCutApplies(1,7444,13206,7704)&&
           !gloveisolate::TmdHardwareCutApplies(2,7444,0,4402),
           "hardware filter is gated to the exact mixed section in isolate mode");
    unsigned previous=0;bool runsValid=true;
    for(const auto& run:gloveisolate::kTmdHardwareFaces){
        if(run.first<previous||run.last<run.first||run.last>=7704)runsValid=false;
        previous=run.last+1;
    }
    Expect(runsValid,"TMD hardware face runs are disjoint and within material 1");
    Expect(gloveisolate::TmdHardwareTrackedSide()==0&&
           gloveisolate::TmdHardwareDelta(0)==0&&
           gloveisolate::TmdHardwareDelta(1)==1,
           "native left TMD hardware shares physical left wrist delta in both handedness modes");
    Expect(gloveisolate::IsTmdDevice("tmd_base")&&gloveisolate::IsTmdDevice("hinge1")&&
           gloveisolate::IsTmdDevice("rail_rot")&&gloveisolate::IsTmdDevice("nixie1")&&
           gloveisolate::IsTmdDevice("faceplate")&&gloveisolate::IsTmdDevice("wheel1"),
           "original animated TMD palette entries remain hardware");

    gloveisolate::Mat4 stump;
    const float wrist[3] = { 10.0f, 0.0f, 0.0f };
    gloveisolate::CollapseDelta(wrist, stump, 0.12f);
    Expect(stump.m[0][0] > 0.0f && stump.m[0][0] < 1.0f,
           "legacy mode 3 stump remains a scale toward the wrist");

    Expect(gloveisolate::NoTmdGlovePaletteCovers(6), "left clavicle is in the glove palette");
    Expect(gloveisolate::NoTmdGlovePaletteCovers(7), "left shoulder is in the glove palette");
    Expect(gloveisolate::NoTmdGlovePaletteCovers(8), "left elbow is in the glove palette");
    Expect(gloveisolate::NoTmdGlovePaletteCovers(9), "left wrist is in the glove palette");
    Expect(gloveisolate::NoTmdGlovePaletteCovers(26), "left forearm is in the glove palette");
    Expect(!gloveisolate::NoTmdGlovePaletteCovers(27),
           "tmd_base is not wired into the pistol glove palette slot");
    Expect(gloveisolate::NoTmdGlovePaletteCovers(41), "left sleeve_fix is in the glove palette");
    Expect(gloveisolate::NoTmdGlovePaletteCovers(46), "left forearm2 is in the glove palette");
    Expect(gloveisolate::NoTmdGlovePaletteCovers(47), "left bicep is in the glove palette");
    Expect(gloveisolate::kNoTmdGlovePaletteBoneCount == 45u,
           "pistol glove palette slot count");
    return g_fails == 0 ? 0 : 1;
}
