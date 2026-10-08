#pragma once

#include <cmath>

namespace weaponwheel {

inline int ClampSlots(int slots) {
    return slots < 2 ? 2 : (slots > 8 ? 8 : slots);
}

// Right-stick direction to a wheel sector. Sector zero is straight up; the
// following sectors proceed clockwise. The dead zone keeps a resting stick
// from changing the highlighted weapon.
inline int SectorForStick(float x, float y, int slots, float deadzone = 0.35f) {
    slots = ClampSlots(slots);
    if (x*x + y*y < deadzone*deadzone) return -1;
    const float pi = 3.14159265358979323846f;
    float a = std::atan2(x, -y); // zero at up, clockwise in screen space
    if (a < 0.0f) a += 2.0f*pi;
    int s = (int)std::floor((a + pi / (float)slots) * (float)slots / (2.0f*pi));
    if (s >= slots) s = 0;
    return s;
}

} // namespace weaponwheel
