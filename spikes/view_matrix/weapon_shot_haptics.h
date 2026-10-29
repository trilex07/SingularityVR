#pragma once
#include <cstdint>

// Game-thread shot witness -> OpenXR-thread bounded amplitude. No controller polling.
namespace weaponhaptics {
enum class Weapon { Pistol, Rifle, Shotgun, Sniper, Other };
struct Profile { float amplitude; std::uint32_t ms; };
inline Profile For(Weapon w) {
    switch (w) {
    case Weapon::Pistol:  return {0.46f, 70};
    case Weapon::Rifle:   return {0.38f, 45};
    case Weapon::Shotgun: return {0.78f, 105};
    case Weapon::Sniper:  return {0.65f, 95};
    default:             return {0.0f, 0};
    }
}
inline Weapon FromVertices(unsigned v) {
    switch (v) {
    case 5799: return Weapon::Pistol;
    case 11387: return Weapon::Rifle;
    case 8297: return Weapon::Shotgun;
    case 11389: case 11390: return Weapon::Sniper;
    default: return Weapon::Other;
    }
}
inline bool ShotCountAdvanced(int before, int after) {
    // A single synchronous native FireAmmunition call may only account for one shot.
    return before >= 0 && std::int64_t(after) == std::int64_t(before) + 1;
}
class Engine {
    std::uint64_t end_ = 0;
    float amp_ = 0;
public:
    bool OnNativeReturn(int before, int after, Weapon w, std::uint64_t now) {
        if (!ShotCountAdvanced(before, after)) return false;
        const Profile p = For(w);
        if (!p.ms) return false;
        amp_ = p.amplitude;
        end_ = now + p.ms;
        return true;
    }
    float Sample(bool gates, float nativeAmp, std::uint64_t now) {
        if (!gates) { Clear(); return 0; }
        if (now >= end_) { amp_ = 0; return 0; }
        // Native vibration is authoritative. Cancel fallback permanently for
        // this shot if it arrives after the initial synthesized frame.
        if (nativeAmp > 0.02f) { Clear(); return 0; }
        return amp_;
    }
    void Clear() { end_ = 0; amp_ = 0; }
};
} // namespace weaponhaptics
