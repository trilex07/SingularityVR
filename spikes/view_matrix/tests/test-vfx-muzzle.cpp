#include "../vfx_muzzle.h"
#include <cstdio>

static int g_fails = 0;
static void Expect(bool cond, const char* name) {
    if (cond) { std::printf("PASS %s\n", name); return; }
    std::printf("FAIL %s\n", name);
    ++g_fails;
}

int main() {
    using namespace vfxmuzzle;
    const int fb = 150;
    // Known guns resolve from the table, unknown fall back to the global knob.
    Expect(ForwardMmForVerts(5799, fb) == 150, "pistol keeps current default");
    Expect(ForwardMmForVerts(11387, fb) == 300, "rifle longer than pistol");
    Expect(ForwardMmForVerts(8297, fb) == 200, "shotgun between");
    Expect(ForwardMmForVerts(11389, fb) == 400, "sniper longest");
    Expect(ForwardMmForVerts(0, fb) == fb, "no gun falls back");
    Expect(ForwardMmForVerts(99999, fb) == fb, "unknown gun falls back");
    Expect(ForwardMmForVerts(99999, 75) == 75, "fallback value honored");
    // Rifles must not inherit the pistol length (the reported defect).
    Expect(ForwardMmForVerts(11387, fb) > ForwardMmForVerts(5799, fb),
           "rifle longer than pistol");
    // UP knob bounds mirror the forward knob (0..200).
    Expect(ClampUpMm(-10) == 0, "up clamps low");
    Expect(ClampUpMm(0) == 0, "up default passes through");
    Expect(ClampUpMm(100) == 100, "up midrange passes through");
    Expect(ClampUpMm(250) == 200, "up clamps high");
    if (g_fails == 0) std::puts("vfx muzzle: geometry gate L1");
    return g_fails == 0 ? 0 : 1;
}
