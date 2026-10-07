#pragma once
#include <cmath>
#include <cstdint>

// Pistol support-grip latch state machine (production-pose-agnostic).
//
// Intended production behaviour (main agent owns the integration; this
// helper owns the LOGIC so it can be tested without a headset):
//   left Grip acquires pistol support, then the support hand remains
//   attached until Grip RELEASE.
// KHARVOX reference: captureRadius 0.20 m for all long guns, grip press
// >0.55 / release <=0.55, support untracked -> hold one-hand + status,
// primary invalid -> hold last pose (OpenXRBootstrap.cpp:1575-1975,
// config/two_hand_weapon_profiles_default.cfg).
//
// Inputs are abstract (distance in metres, booleans, caller clock), so this
// compiles and tests with zero OpenXR/HMD dependency. The caller computes
// the support-target distance (cf. armik::SupportTarget) and supplies it.
//
// Contract:
//   No Grip, or press with no weapon context -> Idle (NEVER latched)
//   Grip press outside radius               -> Searching (held, looking)
//   move inside radius while held           -> Latched
//   move far away while still held          -> STILL LATCHED (until release)
//   Grip release (even still near pistol)   -> immediate Idle
//   still near pistol after release         -> remain Idle (no sticky latch)
//   tracking loss while held (latched)      -> grace: latch preserved
//   tracking restored while Grip held       -> latch preserved
//   tracking loss past timeout              -> Idle
//   weapon switch                           -> Idle

namespace griplatch {

struct Config {
    float acquireRadius = 0.15f; // metres; cf. KHARVOX 0.20 (tune in-headset)
    std::uint64_t trackingGraceMs = 500;
};

enum class State { Idle, Searching, Latched, Grace };

class GripLatch {
public:
    explicit GripLatch(Config cfg = Config{}) : cfg_(cfg) {}
    void Reset() {
        state_ = State::Idle;
        weapon_ = 0;
        haveWeapon_ = false;
        graceStart_ = 0;
    }
    State Get() const { return state_; }
    bool Latched() const { return state_ == State::Latched; }

    State Update(bool gripHeld, float supportDistanceM, bool trackingValid,
                 std::uint32_t weaponId, std::uint64_t nowMs) {
        if (!std::isfinite(supportDistanceM)) supportDistanceM = 1e9f;
        bool weaponChanged = false;
        if (!haveWeapon_ || weaponId != weapon_) {
            // New session or weapon switch -> unlatch (caller re-acquires).
            weapon_ = weaponId;
            haveWeapon_ = true;
            state_ = State::Idle;
            weaponChanged = true;
        }
        // No grip -> immediate Idle (never latched, never searching)
        if (!gripHeld) {
            state_ = State::Idle;
            return state_;
        }
        // A newly selected weapon must not latch on the same input sample that invalidated
        // the previous weapon. Require a fresh in-range update.
        if (weaponChanged) return state_;
        switch (state_) {
        case State::Idle:
            if (trackingValid && supportDistanceM <= cfg_.acquireRadius)
                state_ = State::Latched;
            else
                state_ = State::Searching; // grip held but outside radius or not tracked
            return state_;
        case State::Searching:
            if (!trackingValid) {
                state_ = State::Grace;
                graceStart_ = nowMs;
            } else if (supportDistanceM <= cfg_.acquireRadius) {
                state_ = State::Latched;
            }
            return state_;
        case State::Latched:
            if (!trackingValid) {
                state_ = State::Grace;
                graceStart_ = nowMs;
            }
            // NOTE: distance intentionally ignored while held — moving far away
            // with Grip still down stays attached by design.
            return state_;
        case State::Grace:
            if (!gripHeld) {
                state_ = State::Idle;
            } else if (trackingValid) {
                state_ = State::Latched; // restored while held -> preserved
            } else if (nowMs - graceStart_ >= cfg_.trackingGraceMs) {
                state_ = State::Idle; // loss past timeout -> unlatch
            }
            return state_;
        }
        return state_;
    }

private:
    Config cfg_;
    State state_ = State::Idle;
    std::uint32_t weapon_ = 0;
    bool haveWeapon_ = false;
    std::uint64_t graceStart_ = 0;
};

} // namespace griplatch
