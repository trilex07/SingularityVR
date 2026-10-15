#pragma once

#include <cmath>

namespace weaponwheel {

enum class Event { None, Open, SelectionChanged, Commit, Cancel };

struct State {
    bool open = false;
    bool priorR3 = false;
    int selection = 0;
    // Set on the first highlight change of one open. Release commits only a
    // touched wheel: tapping R3 without ever leaving the deadzone cancels
    // instead of committing the resting sector. Shared by stick and hand.
    bool touched = false;
    // Runtime logging latch (input thread): first SelectionChanged per open.
    bool loggedSel = false;
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
//
// handSector selects the input source: -2 (default) keeps the legacy right
// stick; -1 means "hand inside deadzone / no reading" (highlight unchanged);
// >= 0 is the hand-driven sector. The desktop simulator passes -2 (stick).
inline Event Update(State& state, bool r3Held, bool l3Held, bool gameplay,
                    bool weaponInHands, bool holstered, float stickX, float stickY,
                    int slots, int handSector = -2) {
    const bool pressed = r3Held && !state.priorR3;
    const bool released = !r3Held && state.priorR3;
    state.priorR3 = r3Held;
    const bool valid = gameplay && weaponInHands;

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
            state.touched = false;
            state.loggedSel = false;
            return Event::Open;
        }
        return Event::None;
    }

    if (!r3Held || !valid) {
        const bool touched = state.touched;
        state.open = false;
        state.selection = 0;
        // Commit only a touched wheel: every sector holds a real weapon now,
        // so an untouched release (tap, stick/hand never left the deadzone)
        // must cancel rather than commit the resting sector.
        if (released && valid && touched) return Event::Commit;
        return Event::Cancel;
    }

    const int sector = (handSector != -2) ? handSector
                                                : SectorForStick(stickX, stickY, slots);
    if (sector >= 0 && sector != state.selection) {
        state.selection = sector;
        state.touched = true;
        return Event::SelectionChanged;
    }
    return Event::None;
}

} // namespace weaponwheel
