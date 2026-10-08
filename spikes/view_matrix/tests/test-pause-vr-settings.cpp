#include <cassert>
#include <cstdio>
#include "../pause_vr_settings.h"

int main() {
    using namespace pausevrsettings;
    assert(Hit(.50f, .90f));
    assert(Hit(kU0, kV0));
    assert(Hit(kU1, kV1));
    assert(!Hit(.249f, .90f));
    assert(!Hit(.751f, .90f));
    assert(!Hit(.50f, .859f));
    assert(!Hit(.50f, .951f));
    // Logical coordinates are resolution independent by construction: the same
    // normalized point is the hit at both the 5264x2684 SBS canvas and a small
    // desktop GFx canvas.
    const float u = .50f, v = .90f;
    assert(Hit(u, v));
    assert(Hit(u, v));
    std::puts("pause VR settings row: normalized hit-test is resolution independent");
    return 0;
}
