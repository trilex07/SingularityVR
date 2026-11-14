#pragma once
#include "hip_knife.h"

namespace deadlockgesture {
using hipknife::V;
enum class Event { None, Armed, Cancelled, ThrowIntent };

struct Frame {
    bool eligible = false, grip = false;
    V head{}, hand{};
    float yaw = 0;
    std::uint64_t ms = 0;
    bool leftHanded = false;
};

class Gesture {
    bool neutral = false, armed = false;
    V start{}, previous{}, anchor{}, forward{};
    std::uint64_t started = 0, sampled = 0, anchored = 0, last = 0;

    Event Cancel() {
        const auto result = armed ? Event::Cancelled : Event::None;
        armed = neutral = false;
        return result;
    }

public:
    bool Active() const { return armed; }

    // Local grip acknowledgement, separate from any native power response.
    float ArmPulse(std::uint64_t now) const {
        return armed && now >= started && now - started < 45 ? .22f : 0.f;
    }

    Event Update(const Frame& f) {
        if (!f.eligible || !hipknife::Finite(f.head) || !hipknife::Finite(f.hand)
            || !std::isfinite(f.yaw)) return Cancel();
        if (armed) {
            if (f.ms <= last || f.ms - last > 80 || f.ms - started > 1500) {
                if (f.ms == last && f.grip) return Event::None;
                return Cancel();
            }
            last = f.ms;
            if (f.grip) {
                if (f.ms - sampled >= 28) {
                    anchor = previous;
                    anchored = sampled;
                    previous = f.hand;
                    sampled = f.ms;
                }
                return Event::None;
            }
            const bool useLatest = f.ms - sampled >= 28;
            const auto window = f.ms - (useLatest ? sampled : anchored);
            const float seconds = float(window) * .001f;
            const float speed = hipknife::Dot(f.hand - (useLatest ? previous : anchor), forward) / seconds;
            const float travel = hipknife::Dot(f.hand - start, forward);
            armed = false;
            neutral = true;
            return window >= 20 && window <= 105 && travel >= .18f && speed >= 1.0f
                ? Event::ThrowIntent : Event::Cancelled;
        }
        if (!f.grip) { neutral = true; return Event::None; }
        if (!neutral) return Event::None;
        neutral = false;
        // Tracking space is +X right and +Z behind. Lock launch direction at
        // acquisition so rotating the head alone cannot generate a throw.
        const hipknife::Q yaw{0, std::sin(f.yaw * .5f), 0, std::cos(f.yaw * .5f)};
        const V shoulder = f.head + hipknife::Rotate(yaw, {f.leftHanded ? -.22f : .22f, -.13f, .12f});
        if (hipknife::Length(f.hand - shoulder) > .14f) return Event::None;
        forward = hipknife::Rotate(yaw, {0, 0, -1});
        start = previous = anchor = f.hand;
        started = sampled = anchored = last = f.ms;
        armed = true;
        return Event::Armed;
    }
};
}
