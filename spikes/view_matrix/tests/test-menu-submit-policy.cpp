#include "../menu_submit_policy.h"
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
    Expect(MenuSubmitForcesFlat(1), "class 1 IIS boot/title stays flat");
    Expect(!MenuSubmitForcesFlat(0), "class 0 does not force flat");
    Expect(!MenuSubmitForcesFlat(2), "class 2 main menu does not force flat");
    Expect(!MenuSubmitForcesFlat(3), "class 3 pause menu does not force flat");
    Expect(!MenuSubmitForcesFlat(4), "class 4 station/dialog does not force flat");
    Expect(InteractiveStationMenu(4, true), "class 4 focused is an interactive station");
    Expect(!InteractiveStationMenu(4, false), "class 4 unfocused is not station-gated");
    Expect(!InteractiveStationMenu(2, true), "class 2 is not the station gate");
    Expect(!MenuSubmitForcesFlat(5), "unknown class does not force flat");
    Expect(!MenuSubmitForcesFlat(-1), "negative class does not force flat");
    Expect(MenuSubmitKind(true, true, false) == 1, "crop submission is its own kind");
    Expect(MenuSubmitKind(false, true, false) == 2, "side-by-side halves are not a crop");
    Expect(MenuSubmitKind(false, true, true) == 2, "duplicated stereo still uses half rects");
    Expect(MenuSubmitKind(false, false, true) == 3, "stereo without duplication is per-eye");
    Expect(MenuSubmitKind(false, false, false) == 4, "neither flag is mono");
    return g_fails == 0 ? 0 : 1;
}
