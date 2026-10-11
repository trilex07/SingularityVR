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
    Expect(!MenuSubmitForcesFlat(5), "unknown class does not force flat");
    Expect(!MenuSubmitForcesFlat(-1), "negative class does not force flat");
    return g_fails == 0 ? 0 : 1;
}
