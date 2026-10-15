#include "../weapon_holster.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <initializer_list>

static bool close(float a, float b, float e = 1e-5f) { return std::fabs(a - b) <= e; }

int main() {
    using namespace weaponholster;
    const Config c{4.0f, 18.0f, -24.0f, 12.0f};
    const V3 hip{4.0f, 18.0f, -24.0f};
    State s;

    assert(close(DistanceSq(hip, hip), 0.0f));
    assert(NearHip(hip, hip, c));
    assert(NearHip({4.0f, 18.0f, -12.01f}, hip, c));
    assert(!NearHip({4.0f, 18.0f, -11.9f}, hip, c));
    for (float yaw : {0.0f, 0.7f, 1.9f, 3.14159265f, -2.4f}) {
        const V3 rotated = HipPoint(c, yaw);
        assert(close(DistanceSq(rotated, rotated), 0.0f));
        assert(NearHip(rotated, rotated, c));
    }

    assert(Update(s, false, true, true, 52, hip, hip, c) == Event::None);
    assert(!s.active && s.weapon == 52);
    assert(Update(s, true, true, true, 52, hip, hip, c) == Event::Stored);
    assert(s.active);
    // Holding the grip is not a repeated toggle.
    assert(Update(s, false, true, true, 52, hip, hip, c) == Event::None);
    assert(s.active);
    // Leaving and re-entering the hip with a new rising edge draws the gun.
    assert(Update(s, true, true, true, 52, {80.0f, 80.0f, 80.0f}, hip, c) == Event::None);
    assert(Update(s, true, true, true, 52, hip, hip, c) == Event::Drawn);
    assert(!s.active);
    // A different weapon clears the old holster state before it can be used.
    assert(Update(s, true, true, true, 57, hip, hip, c) == Event::Reset);
    assert(!s.active && s.weapon == 57);
    // Menus/tracking loss reset the state and never toggle.
    s.active = true;
    assert(Update(s, true, false, true, 57, hip, hip, c) == Event::Reset);
    assert(!s.active && s.weapon == 0);
    // MAIN round 20: store needs radius (12), draw accepts drawRadius (20).
    {
        State d; const V3 off{4.0f, 18.0f + 16.0f, -24.0f};  // 16 UU from hip
        assert(Update(d, true, true, true, 9, off, hip, c) == Event::None);   // too far to store
        assert(Update(d, true, true, true, 9, hip, hip, c) == Event::Stored);
        assert(Update(d, true, true, true, 9, off, hip, c) == Event::Drawn);  // 16 < 20 draws
        assert(Update(d, true, true, true, 9, {4.0f, 18.0f + 21.0f, -24.0f}, hip, c) == Event::None);
    }
    std::puts("weapon holster: radius, rising-edge toggle, hold, re-entry, weapon/reset gates, drawRadius");
}
