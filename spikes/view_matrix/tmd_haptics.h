#pragma once
#include <cstdint>
#include <cstring>

// TMD power haptics: pure, allocation-free policy.
//
// Generates a single overlay amplitude (0..1) for the TMD-holding hand (OffHand()).
// The existing rumble loop in d3d9.cpp (~12350) takes max(native XInput rumble, overlay)
// for the off-hand, preserving the game's authored weapon feedback on the primary hand
// and avoiding any second OpenXR vibration call or altered 15-frame refresh timing.
//
// Speculative sustain has been strictly removed: all powers produce bounded, crisp
// short pulses (<= 120 ms) triggered ONLY on confirmed native power acceptance.
namespace tmdhaptics {

enum class Power : std::uint8_t { None, Impulse, Age, AntiGrav, Deadlock, Heal, Shield, Teleport, Generic };
enum class Level : std::uint8_t { Low = 0, Normal = 1, Strong = 2 };

struct Config {
    bool enabled = true;           // [Haptics] Enabled - master switch for ALL controller vibration
    Level level = Level::Normal;   // NORMAL is the identity scale: existing weapon rumble unchanged
    bool tmd = false;              // [Haptics] TmdPulses - default OFF until verified in physical play
};

inline float Scale(Level l) {
    return l == Level::Low ? 0.5f : (l == Level::Strong ? 1.3f : 1.0f);
}
inline float Clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
inline float ApplyLevel(float amp, const Config& c) {
    if (!c.enabled) return 0.0f;
    return Clamp01(amp * Scale(c.level));
}

struct Profile {
    float startAmp;         // pulse on confirmed accepted start
    std::uint32_t startMs;  // duration in ms (bounded <= 120 ms)
};

inline Power ClassifyExactName(const char* name) {
    if (!name || !name[0]) return Power::None;
    struct Entry { const char* name; Power power; };
    static const Entry names[] = {
        {"RvTMDPower_Impulse", Power::Impulse}, {"RvTMDPower_Age", Power::Age},
        {"RvTMDPower_AntiGrav", Power::AntiGrav}, {"RvTMDPower_Deadlock", Power::Deadlock},
        {"RvTMDPower_Heal", Power::Heal}, {"RvTMDPower_Shield", Power::Shield},
        {"RvTMDPower_Teleport", Power::Teleport}
    };
    for (const auto& e : names) if (std::strcmp(name, e.name) == 0) return e.power;
    return Power::None;
}

// The native hook has already returned from the real method. Unknown class
// names are allowed only for a readable receiver AND a positively owned TMD.
inline Power SelectLocalPower(Power classified, bool readableReceiver, bool confirmedLocal) {
    if (!readableReceiver || !confirmedLocal) return Power::None;
    return classified == Power::None ? Power::Generic : classified;
}

// Verified conservative short pulses. Every amplitude <= 0.80 and duration <= 100 ms.
// No speculative continuous sustain vibration.
inline Profile ProfileFor(Power p) {
    switch (p) {
    case Power::Impulse:  return { 0.80f, 90 };  // strong, short impulse
    case Power::Age:      return { 0.50f, 90 };  // distinct medium pulse
    case Power::Teleport: return { 0.60f, 80 };  // snappy teleport jolt
    case Power::AntiGrav: return { 0.40f, 70 };  // subtle anti-grav trigger
    case Power::Deadlock: return { 0.55f, 80 };  // distinct deadlock launch
    case Power::Heal:     return { 0.45f, 70 };  // subtle heal pulse
    case Power::Shield:   return { 0.50f, 70 };  // medium shield pulse
    case Power::Generic:  return { 0.40f, 70 };  // local-only fallback; no sustained rumble
    case Power::None:     break;
    }
    return { 0.0f, 0 };
}

constexpr std::uint64_t kDuplicateMs = 30; // duplicate native events within 30 ms ignored

class Engine {
public:
    // accepted: native Start path confirmed activation (ret == 0: energy and cooldown passed).
    // Returns true when an overlay pulse was armed.
    bool OnStart(Power p, bool accepted, const Config& c, std::uint64_t now) {
        if (!c.enabled || !c.tmd || p == Power::None || !accepted) return false;
        if (p == lastStart_ && (now - lastStartAt_ < kDuplicateMs) && now >= lastStartAt_) return false;
        lastStart_ = p;
        lastStartAt_ = now;
        const Profile pr = ProfileFor(p);
        if (now >= pulseEnd_ || pr.startAmp >= pulseAmp_) {
            pulseAmp_ = pr.startAmp;
            pulseEnd_ = now + pr.startMs;
        } else if (now + pr.startMs > pulseEnd_) {
            pulseEnd_ = now + pr.startMs;
        }
        return true;
    }

    bool OnStop(Power, const Config&, std::uint64_t) {
        // Speculative sustain removed: StopFirePower does not need to cancel or pulse
        return false;
    }

    void Clear() {
        pulseAmp_ = 0.0f;
        pulseEnd_ = 0;
        lastStart_ = Power::None;
        lastStartAt_ = 0;
    }

    // gatesOk: gameplay active, focused, TMD hand tracked/held, no pause/cinematic/menu.
    float Amplitude(const Config& c, bool gatesOk, std::uint64_t now) {
        if (!c.enabled || !c.tmd || !gatesOk) {
            Clear();
            return 0.0f;
        }
        if (now < pulseEnd_) return pulseAmp_;
        pulseAmp_ = 0.0f;
        return 0.0f;
    }

    bool IsPulsing(std::uint64_t now) const {
        return now < pulseEnd_ && pulseAmp_ > 0.0f;
    }

private:
    float pulseAmp_ = 0.0f;
    std::uint64_t pulseEnd_ = 0;
    Power lastStart_ = Power::None;
    std::uint64_t lastStartAt_ = 0;
};

// Per-hand merge used by the single rumble site in d3d9.cpp.
// nativeAmp is the game's authored XInput rumble; overlay applies only to isTmdHand.
// Result is never below nativeAmp (before level scaling).
inline float Merge(float nativeAmp, float overlay, bool isTmdHand, const Config& c) {
    const float a = (isTmdHand && overlay > nativeAmp) ? overlay : nativeAmp;
    return ApplyLevel(a, c);
}

} // namespace tmdhaptics
