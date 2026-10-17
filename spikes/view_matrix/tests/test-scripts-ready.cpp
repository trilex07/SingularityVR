#include "../scripts_ready.h"
#include <cstdio>

static int g_fails = 0;
static void Expect(bool cond, const char* name) {
    if (cond) { std::printf("PASS %s\n", name); return; }
    std::printf("FAIL %s\n", name);
    ++g_fails;
}

int main() {
    using namespace scriptsready;
    const Signals none{ false, false, false };
    const Signals hookOnly{ true, false, false };
    const Signals offsetsOnly{ false, true, false };
    const Signals armedOnly{ false, false, true };
    const Signals hookOffsets{ true, true, false };
    const Signals hookArmed{ true, false, true };
    const Signals offsetsArmed{ false, true, true };
    const Signals all{ true, true, true };
    Expect(!Ready(none), "nothing ready => not ready");
    Expect(!Ready(hookOnly), "hook alone is not enough");
    Expect(!Ready(offsetsOnly), "offsets alone are not enough");
    Expect(!Ready(armedOnly), "armed alone is not enough");
    Expect(!Ready(hookOffsets), "missing armed => not ready");
    Expect(!Ready(hookArmed), "missing offsets => not ready");
    Expect(!Ready(offsetsArmed), "missing route hook => not ready");
    Expect(Ready(all), "all four signals => ready");
    // Hold decision matrix.
    Expect(!HoldActive(false, false, 0), "disabled never holds");
    Expect(!HoldActive(true, true, 0), "latched ready never holds");
    Expect(HoldActive(true, false, 0), "holds from the first unready frame");
    Expect(HoldActive(true, false, HoldTimeoutFrames() - 1), "holds until timeout");
    Expect(!HoldActive(true, false, HoldTimeoutFrames()), "timeout releases at the bound");
    Expect(!HoldActive(true, false, HoldTimeoutFrames() + 100), "timeout stays released");
    Expect(HoldTimeoutFrames() == 1800, "timeout bound documented");
    if (g_fails == 0) std::puts("scripts readiness: gate L1");
    return g_fails == 0 ? 0 : 1;
}
