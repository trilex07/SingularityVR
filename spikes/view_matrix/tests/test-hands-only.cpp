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

    // Hands-only does not solve elbows. The cuff is the stump scale toward the
    // wrist, which leaves the glove bones unscaled.
    gloveisolate::Mat4 stump;
    const float wrist[3] = { 10.0f, 0.0f, 0.0f };
    gloveisolate::CollapseDelta(wrist, stump, 0.12f);
    Expect(stump.m[0][0] > 0.0f && stump.m[0][0] < 1.0f && stump.m[3][3] == 1.0f,
           "the short cuff scales toward the wrist and does not build an elbow");
    return g_fails == 0 ? 0 : 1;
}
