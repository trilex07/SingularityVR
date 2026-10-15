#pragma once

#include <cmath>
#include <cstdint>

// Small, allocation-free state machine for the motion-controller holster.
// Runtime code supplies a world-space hand position and a body-relative hip
// point; this header intentionally knows nothing about OpenXR or D3D.
namespace weaponholster {

struct V3 { float x, y, z; };

struct Config {
    float fwd = 4.0f;       // UU forward from the body origin
    float right = 18.0f;    // UU to the primary-hand side
    float up = -24.0f;      // UU below the view/body origin
    float radius = 12.0f;   // UU activation radius (store)
    // MAIN round 20/21: draw uses a wider sphere. r19/r20 user: storing fine, drawing
    // needs a wider tolerance so player doesn't have to hit a tiny point.
    float drawRadius = 32.0f;
};

enum class Event { None, Stored, Drawn, Reset };

struct State {
    bool active = false;
    std::uint32_t weapon = 0;
};

inline float DistanceSq(V3 a, V3 b) {
    const float x = a.x - b.x, y = a.y - b.y, z = a.z - b.z;
    return x*x + y*y + z*z;
}

inline bool NearHip(V3 hand, V3 hip, const Config& c) {
    return DistanceSq(hand, hip) <= c.radius * c.radius;
}

inline V3 HipPoint(const Config& c, float bodyYawRad) {
    const float cy = std::cos(bodyYawRad), sy = std::sin(bodyYawRad);
    return {c.fwd * cy - c.right * sy,
            c.fwd * sy + c.right * cy,
            c.up};
}

// A grip rising edge near the hip toggles the current weapon.  A weapon or
// gameplay change invalidates the latch so a stale holster can never affect a
// newly equipped item or a menu/cutscene.
inline Event Update(State& s, bool gripRising, bool valid, bool gameplay,
                    std::uint32_t weapon, V3 hand, V3 hip, const Config& c) {
    if (!valid || !gameplay || weapon == 0) {
        if (s.active || s.weapon != 0) {
            s = {};
            return Event::Reset;
        }
        return Event::None;
    }
    if (s.weapon != 0 && s.weapon != weapon) {
        s.active = false;
        s.weapon = weapon;
        return Event::Reset;
    }
    s.weapon = weapon;
    if (!gripRising) return Event::None;
    const float r = s.active ? c.drawRadius : c.radius;
    if (DistanceSq(hand, hip) > r * r) return Event::None;
    s.active = !s.active;
    return s.active ? Event::Stored : Event::Drawn;
}

} // namespace weaponholster
