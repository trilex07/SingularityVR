#pragma once

#include <cmath>
#include <cstdint>

// Gesture-only reload recognizer.  This is deliberately independent of
// OpenXR, XInput and the game ABI: it emits one request, while the caller
// remains responsible for invoking the game's native reload path.
namespace manualreload {

struct Vec3 {
    float x = 0, y = 0, z = 0;
};

inline float Distance(Vec3 a, Vec3 b) {
    const float x = a.x - b.x, y = a.y - b.y, z = a.z - b.z;
    return std::sqrt(x * x + y * y + z * z);
}

struct Config {
    float approachRadiusM = 0.11f; // hand must enter the reload socket
    float pullDistanceM = 0.16f;   // then move away to eject/pull
    float insertRadiusM = 0.10f;   // return to the socket to finish gesture
    std::uint64_t requestTimeoutMs = 350;
    std::uint64_t cooldownMs = 350;
};

enum class State { Idle, Candidate, Pulled, WaitingNative, Cooldown };

struct Input {
    bool weaponValid = false;
    bool trackingValid = false;
    bool supportGripLatched = false;
    bool firing = false;
    bool nativeReloading = false;
    std::uint32_t weaponId = 0;
    Vec3 hand{};
    Vec3 reloadAnchor{};
    std::uint64_t nowMs = 0;
};

struct Result {
    State state = State::Idle;
    bool requestNativeReload = false;
};

class Gesture {
public:
    explicit Gesture(Config cfg = Config{}) : cfg_(cfg) {}

    void Reset() {
        state_ = State::Idle;
        haveWeapon_ = false;
        weaponId_ = 0;
        stateSinceMs_ = 0;
    }

    State GetState() const { return state_; }

    // Returns requestNativeReload on exactly the pull-away -> return edge.
    // No ammo, animation or input state is changed here.
    Result Update(const Input& in) {
        Result out;
        out.state = state_;

        bool weaponChanged = false;
        if (!haveWeapon_ || weaponId_ != in.weaponId) {
            haveWeapon_ = in.weaponValid;
            weaponId_ = in.weaponId;
            state_ = State::Idle;
            stateSinceMs_ = in.nowMs;
            weaponChanged = true;
        }
        if (!in.weaponValid || !in.trackingValid || in.supportGripLatched || in.firing) {
            // A support-grip latch owns the left hand.  It must never be
            // interpreted as a reload gesture while the weapon is held.
            if (state_ != State::WaitingNative) state_ = State::Idle;
            stateSinceMs_ = in.nowMs;
            out.state = state_;
            return out;
        }
        // Do not let a weapon swap plus a hand already sitting at the socket
        // silently create a reload. Require one clean sample for the new gun.
        if (weaponChanged) {
            out.state = state_;
            return out;
        }

        if (state_ == State::WaitingNative) {
            if (in.nativeReloading) {
                out.state = state_;
                return out;
            }
            if (in.nowMs - stateSinceMs_ >= cfg_.requestTimeoutMs) {
                state_ = State::Cooldown;
                stateSinceMs_ = in.nowMs;
            }
            out.state = state_;
            return out;
        }
        if (state_ == State::Cooldown) {
            if (in.nowMs - stateSinceMs_ >= cfg_.cooldownMs) {
                state_ = State::Idle;
                stateSinceMs_ = in.nowMs;
            }
            out.state = state_;
            return out;
        }

        const float d = Distance(in.hand, in.reloadAnchor);
        if (!std::isfinite(d)) {
            state_ = State::Idle;
            out.state = state_;
            return out;
        }
        switch (state_) {
        case State::Idle:
            if (d <= cfg_.approachRadiusM) {
                state_ = State::Candidate;
                stateSinceMs_ = in.nowMs;
            }
            break;
        case State::Candidate:
            if (d >= cfg_.pullDistanceM) {
                state_ = State::Pulled;
                stateSinceMs_ = in.nowMs;
            } else if (in.nowMs - stateSinceMs_ >= cfg_.requestTimeoutMs) {
                state_ = State::Idle;
            }
            break;
        case State::Pulled:
            if (d <= cfg_.insertRadiusM) {
                state_ = State::WaitingNative;
                stateSinceMs_ = in.nowMs;
                out.requestNativeReload = true;
            }
            break;
        case State::WaitingNative:
        case State::Cooldown:
            break;
        }
        out.state = state_;
        return out;
    }

private:
    Config cfg_;
    State state_ = State::Idle;
    std::uint32_t weaponId_ = 0;
    bool haveWeapon_ = false;
    std::uint64_t stateSinceMs_ = 0;
};

} // namespace manualreload
