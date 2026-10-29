#pragma once
#include <cstdint>

namespace microstutter {
enum Event : std::uint32_t {
    Fire = 1u << 0, Damage = 1u << 1, WeaponSwitch = 1u << 2,
    InteractionAttempt = 1u << 3
};

// These are observations, not causal labels. In particular, a Use press is
// not proof that a world item was picked up.
struct Sample {
    std::uint32_t frame = 0, thread = 0, events = 0;
    std::uint32_t frameWallUs = 0, hookWallUs = 0, presentCallUs = 0;
    std::uint32_t renderThreadCpuUs = 0, fireNativeMaxUs = 0, damageNativeMaxUs = 0;
    std::uint64_t qpc = 0;
};

inline bool IsSpike(const Sample& s, std::uint32_t thresholdUs = 11111) {
    return s.frameWallUs > thresholdUs;
}
inline bool IsEvent(const Sample& s, Event e) { return (s.events & e) != 0; }
inline std::uint32_t UnexplainedWallUs(const Sample& s) {
    // This residual includes game work, GPU/compositor waits and scheduling.
    // It must never be called "CPU time".
    return s.frameWallUs > s.hookWallUs ? s.frameWallUs - s.hookWallUs : 0;
}
}
