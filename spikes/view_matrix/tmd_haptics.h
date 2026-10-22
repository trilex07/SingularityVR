#pragma once
#include <cstdint>

// TMD power haptics: pure, allocation-free policy. It does NOT talk to OpenXR.
//
// It produces ONE overlay amplitude (0..1) for the TMD-holding hand. The existing rumble loop in
// d3d9.cpp (the single xrApplyHapticFeedback / xrStopHapticFeedback site, 200 ms pulses refreshed
// every 15 frames) takes max(native XInput rumble, overlay) per hand, so there is no second
// vibration loop and no second OpenXR call site.
//
// Events come ONLY from the verified native StartFirePower / StopFirePower methods (see
// native_tmd_haptics.h). A button press alone never produces a pulse: Start must report that the
// native activation path ran (accepted == true). Bounded state: one pulse slot + one sustain slot.
namespace tmdhaptics {

enum class Power : std::uint8_t { None, Impulse, Age, AntiGrav, Deadlock, Heal, Shield, Teleport };
enum class Level : std::uint8_t { Low = 0, Normal = 1, Strong = 2 };

struct Config {
    bool enabled = true;           // [Haptics] Enabled - master switch for ALL controller vibration
    Level level = Level::Normal;   // NORMAL is the identity scale: existing weapon rumble unchanged
    bool tmd = false;              // [Haptics] TmdPulses - default OFF until runtime receipts exist
};

inline float Scale(Level l) {
    return l == Level::Low ? 0.5f : (l == Level::Strong ? 1.3f : 1.0f);
}
inline float Clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
// Applied to the final per-hand amplitude, native rumble included. Normal == 1.0 exactly.
inline float ApplyLevel(float amp, const Config& c) {
    if (!c.enabled) return 0.0f;
    return Clamp01(amp * Scale(c.level));
}

struct Profile {
    float startAmp;         // pulse on accepted start
    std::uint32_t startMs;
    float sustainAmp;       // 0 = none
    float endAmp;           // pulse on end of a genuinely active power; 0 = none
    std::uint32_t endMs;
};

// Conservative; every amplitude <= 0.85 and every pulse <= 120 ms, sustain <= 0.12.
inline Profile ProfileFor(Power p) {
    switch (p) {
    case Power::Impulse:  return { 0.80f, 90, 0.00f, 0.00f, 0 };   // strong, short
    case Power::Age:      return { 0.50f, 120, 0.08f, 0.25f, 40 }; // distinct medium
    case Power::Teleport: return { 0.60f, 80, 0.00f, 0.00f, 0 };
    case Power::AntiGrav: return { 0.35f, 60, 0.10f, 0.25f, 40 };
    case Power::Deadlock: return { 0.50f, 70, 0.12f, 0.30f, 50 };
    case Power::Heal:     return { 0.40f, 70, 0.10f, 0.25f, 40 };
    case Power::Shield:   return { 0.45f, 70, 0.10f, 0.25f, 40 };
    case Power::None:     break;
    }
    return { 0, 0, 0, 0, 0 };
}

constexpr std::uint64_t kSustainCapMs = 10000;  // watchdog: a missed Stop can never stick
constexpr std::uint64_t kDuplicateMs = 30;      // one native event never pulses twice

class Engine {
public:
    // accepted: the native Start path reported it ACTIVATED the power (not blocked / no energy).
    // Returns true when an overlay pulse was armed.
    bool OnStart(Power p, bool accepted, const Config& c, std::uint64_t now) {
        if (!c.enabled || !c.tmd || p == Power::None || !accepted) return false;
        if (p == lastStart_ && now - lastStartAt_ < kDuplicateMs && now >= lastStartAt_) return false;
        lastStart_ = p; lastStartAt_ = now;
        const Profile pr = ProfileFor(p);
        if (pr.sustainAmp > 0.0f) { active_ = p; activeSince_ = now; }
        else if (active_ != Power::None) active_ = Power::None;   // new instant power replaces
        // Rapid successive powers: newest wins unless the pulse still playing is stronger.
        if (now >= pulseEnd_ || pr.startAmp >= pulseAmp_) { pulseAmp_ = pr.startAmp; pulseEnd_ = now + pr.startMs; }
        return true;
    }
    // Stop is only a "completion" when that power was genuinely active.
    bool OnStop(Power p, const Config& c, std::uint64_t now) {
        if (p == Power::None || active_ != p) return false;
        const Profile pr = ProfileFor(p);
        active_ = Power::None;
        if (!c.enabled || !c.tmd || pr.endAmp <= 0.0f) return false;
        pulseAmp_ = pr.endAmp; pulseEnd_ = now + pr.endMs;
        return true;
    }
    void Clear() { pulseAmp_ = 0; pulseEnd_ = 0; active_ = Power::None; lastStart_ = Power::None; }
    // gatesOk: gameplay active, focused, TMD hand tracked/held. Called once per frame.
    float Amplitude(const Config& c, bool gatesOk, std::uint64_t now) {
        if (!c.enabled || !c.tmd || !gatesOk) { Clear(); return 0.0f; }
        float amp = 0.0f;
        if (now < pulseEnd_) amp = pulseAmp_;
        if (active_ != Power::None) {
            if (now - activeSince_ > kSustainCapMs) active_ = Power::None;
            else { const float s = ProfileFor(active_).sustainAmp; if (s > amp) amp = s; }
        }
        return amp;
    }
    Power Active() const { return active_; }
private:
    float pulseAmp_ = 0; std::uint64_t pulseEnd_ = 0;
    Power active_ = Power::None, lastStart_ = Power::None;
    std::uint64_t activeSince_ = 0, lastStartAt_ = 0;
};

// Per-hand merge used by the one rumble site. nativeAmp is the game's XInput rumble for that
// hand; overlay applies only to tmdHand. Result is never below the native value (before level).
inline float Merge(float nativeAmp, float overlay, bool isTmdHand, const Config& c) {
    const float a = (isTmdHand && overlay > nativeAmp) ? overlay : nativeAmp;
    return ApplyLevel(a, c);
}

}  // namespace tmdhaptics
