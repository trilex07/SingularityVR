#include "../ammo_probe.h"
#include <cstdio>

static int g_fails = 0;
static void Expect(bool cond, const char* name) {
    if (cond) { std::printf("PASS %s\n", name); return; }
    std::printf("FAIL %s\n", name);
    ++g_fails;
}

int main() {
    using namespace ammoprobe;
    Expect(kSlots == 256, "coverage window");
    // Rising edge with budget, latched gun and live object dumps.
    Expect(ShouldDump(true, false, 8, true, 0x12345678), "rising edge dumps");
    Expect(!ShouldDump(true, true, 8, true, 0x12345678), "held trigger silent");
    Expect(!ShouldDump(true, false, 0, true, 0x12345678), "spent budget silent");
    Expect(!ShouldDump(true, false, 8, false, 0x12345678), "no gun no dump");
    Expect(!ShouldDump(true, false, 8, true, 0), "null object no dump");
    Expect(!ShouldDump(false, false, 8, true, 0x12345678), "released trigger silent");
    const int a[4] = { 12, 48, 0, 7 };
    const int b[4] = { 11, 48, 0, 7 };
    const int c[4] = { 12, 48, 0, 7 };
    Expect(DiffCount(a, b, 4) == 1, "single moving slot");
    Expect(DiffCount(a, c, 4) == 0, "identical snapshots");
    if (g_fails == 0) std::puts("ammo probe: edge gate + diff L1");
    return g_fails == 0 ? 0 : 1;
}
