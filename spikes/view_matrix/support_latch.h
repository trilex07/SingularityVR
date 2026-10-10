#pragma once
#include <cmath>

// Deliberate support-grip state.  The capture radius is an acquisition condition only;
// once the off hand has joined the weapon it must not be pulled off by tracking drift.
enum class SupportLatchState { Idle, Searching, Latched, TrackingLostGrace };

struct SupportLatch {
    SupportLatchState state = SupportLatchState::Idle;
    unsigned lostFrames = 0;
};

struct SupportLatchInput {
    bool grip = false;
    bool tracked = false;
    bool weaponValid = false;
    bool withinAcquire = false;
};

inline bool UpdateSupportLatch(SupportLatch& s, const SupportLatchInput& in,
                               unsigned graceFrames = 18) {
    if (!in.weaponValid || !in.grip) {
        s.state = SupportLatchState::Idle;
        s.lostFrames = 0;
        return false;
    }
    switch (s.state) {
    case SupportLatchState::Idle:
        s.lostFrames = 0;
        s.state = in.tracked ? SupportLatchState::Searching : SupportLatchState::TrackingLostGrace;
        break;
    case SupportLatchState::Searching:
        if (in.tracked && in.withinAcquire) s.state = SupportLatchState::Latched;
        else if (!in.tracked) { s.state = SupportLatchState::TrackingLostGrace; s.lostFrames = 1; }
        break;
    case SupportLatchState::Latched:
        if (!in.tracked) { s.state = SupportLatchState::TrackingLostGrace; s.lostFrames = 1; }
        break;
    case SupportLatchState::TrackingLostGrace:
        if (in.tracked) { s.state = SupportLatchState::Latched; s.lostFrames = 0; }
        else if (++s.lostFrames > graceFrames) { s.state = SupportLatchState::Idle; s.lostFrames = 0; }
        break;
    }
    return s.state == SupportLatchState::Latched;
}
