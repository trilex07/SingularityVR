#include "../manual_reload.h"
#include <cassert>
#include <cstdio>

using manualreload::Gesture;
using manualreload::Input;
using manualreload::State;

static Input sample(std::uint64_t t, float distance) {
    Input in;
    in.weaponValid = true;
    in.trackingValid = true;
    in.weaponId = 7;
    in.nowMs = t;
    in.hand = {distance, 0, 0};
    return in;
}

int main() {
    Gesture g;
    auto r = g.Update(sample(0, 0.30f));
    assert(r.state == State::Idle && !r.requestNativeReload);
    r = g.Update(sample(16, 0.08f)); // approach socket
    assert(r.state == State::Candidate);
    r = g.Update(sample(32, 0.20f)); // pull/eject
    assert(r.state == State::Pulled && !r.requestNativeReload);
    r = g.Update(sample(48, 0.05f)); // return/insert
    assert(r.state == State::WaitingNative && r.requestNativeReload);
    r = g.Update(sample(64, 0.05f)); // held in socket: no repeat
    assert(!r.requestNativeReload);
    r = g.Update(sample(420, 0.05f)); // native call not acknowledged
    assert(r.state == State::Cooldown && !r.requestNativeReload);
    r = g.Update(sample(800, 0.05f));
    assert(r.state == State::Idle && !r.requestNativeReload);

    // Support grip owns the hand and blocks the gesture.
    Gesture blocked;
    Input in = sample(0, 0.05f);
    in.supportGripLatched = true;
    assert(blocked.Update(in).state == State::Idle);
    in.supportGripLatched = false;
    in.nowMs = 16; in.hand.x = 0.20f;
    assert(blocked.Update(in).state == State::Idle);

    // Firing cannot create a reload request, and weapon changes re-arm cleanly.
    Gesture guarded;
    in = sample(0, 0.05f); in.firing = true;
    assert(guarded.Update(in).state == State::Idle);
    in.firing = false; in.nowMs = 16; in.hand.x = 0.20f;
    assert(guarded.Update(in).state == State::Idle);
    in.weaponId = 8; in.nowMs = 32; in.hand.x = 0.05f;
    assert(guarded.Update(in).state == State::Idle);

    std::puts("PASS: manual reload approach/pull/insert, no-repeat, timeout, support-grip and firing guards");
    return 0;
}
