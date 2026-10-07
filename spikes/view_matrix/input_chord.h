#pragma once
#include <cstdint>

// L3+R3 recenter chord state machine (OpenXR-agnostic).
//
// Requirement (research brief section 23): production recenter gesture is
// L3 + R3 (both thumbstick clicks). Reference timing: Dishonored-VR fires
// panel on tap (<350 ms) and recenter on hold (600 ms) for its own chord
// (src/core/vr/openxr_input.cpp:557-582); our chord window default (250 ms)
// is a starting point the main agent can tune against the production input
// path — this helper only reports the discrete event.
//
// Contract:
//   L3 alone            -> nothing
//   R3 alone            -> nothing
//   L3 then R3 (or R3 then L3) inside the window -> exactly one event
//   holding both       -> no repeats
//   release both       -> rearm
// Time is caller-supplied milliseconds (frame time or action timestamp), so
// the helper stays independent of OpenXR specifics and is unit-testable.

namespace inputchord {

struct Config {
    std::uint64_t windowMs = 250; // second press must land inside this window
};

class StickChord {
public:
    explicit StickChord(Config cfg = Config{}) : cfg_(cfg) {}
    void Reset() {
        state_ = State::Idle;
        deadline_ = 0;
    }
    // Returns true exactly once per qualifying chord press.
    bool Update(bool l3, bool r3, std::uint64_t nowMs) {
        if (!l3 && !r3) {
            state_ = State::Idle; // both released -> rearm
            return false;
        }
        switch (state_) {
        case State::Idle:
            if (l3 && r3) {
                state_ = State::Held; // simultaneous press: fire immediately
                return true;
            }
            state_ = State::Armed;
            firstLeft_ = l3;
            deadline_ = nowMs + cfg_.windowMs;
            return false;
        case State::Armed:
            if (l3 && r3) {
                const bool inWindow = nowMs <= deadline_;
                state_ = State::Held;
                return inWindow; // late second press still holds, but fires nothing
            }
            if ((l3 && firstLeft_) || (r3 && !firstLeft_)) {
                // First side still held, waiting: expiry keeps it simple —
                // a stale first press never fires on its own.
                if (nowMs > deadline_) state_ = State::Stale;
                return false;
            }
            // First released before the chord completed -> cancel.
            state_ = State::Idle;
            return false;
        case State::Stale:
            if (l3 && r3) {
                state_ = State::Held; // no event: window expired
            } else if (!l3 && !r3) {
                state_ = State::Idle;
            }
            return false;
        case State::Held:
            return false; // holding both -> no repeats
        }
        return false;
    }

private:
    enum class State { Idle, Armed, Stale, Held };
    Config cfg_;
    State state_ = State::Idle;
    bool firstLeft_ = false;
    std::uint64_t deadline_ = 0;
};

} // namespace inputchord
