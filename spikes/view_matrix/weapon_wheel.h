#pragma once

#include <cmath>

namespace weaponwheel {

enum class Event { None, Open, SelectionChanged, Commit, Cancel };

struct State {
    bool open = false;
    bool priorR3 = false;
    int selection = 0;
};

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

// One edge-driven update shared by the runtime and the desktop R3 simulator.
// It owns no engine state: the caller mirrors `open`, `selection`, and commit
// pulses into its native input path.  This makes press/hold/release behaviour
// deterministic and prevents a held R3 from generating repeated commits.
inline Event Update(State& state, bool r3Held, bool l3Held, bool gameplay,
                    bool weaponInHands, bool holstered, float stickX, float stickY,
                    int slots) {
    const bool pressed = r3Held && !state.priorR3;
    const bool released = !r3Held && state.priorR3;
    state.priorR3 = r3Held;
    const bool valid = gameplay && weaponInHands && !holstered;

    if (l3Held) {
        if (state.open) {
            state.open = false;
            state.selection = 0;
            return Event::Cancel;
        }
        return Event::None;
    }
    if (!state.open) {
        if (valid && pressed) {
            state.open = true;
            state.selection = 0;
            return Event::Open;
        }
        return Event::None;
    }

    if (!r3Held || !valid) {
        const int selection = state.selection;
        state.open = false;
        state.selection = 0;
        if (released && valid && selection > 0) return Event::Commit;
        return Event::Cancel;
    }

    const int sector = SectorForStick(stickX, stickY, slots);
    if (sector >= 0 && sector != state.selection) {
        state.selection = sector;
        return Event::SelectionChanged;
    }
    return Event::None;
}

} // namespace weaponwheel
