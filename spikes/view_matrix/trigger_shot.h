#pragma once

// Shot-fired trigger resistance for SingularityVR (PSVR2 Sense).
//
// The base profiles (psvr2_trigger.h, KHARVOX-derived) give each weapon a HOLD
// wall while aiming. This header adds the SHOT itself: a significant,
// short vibration kick on every FireAmmunition event, per weapon, that decays
// back to the hold wall. That two-phase feel (wall + kick) is what makes a
// trigger feel like a gun rather than a stiff button.
//
// Ranges mirror the validated KHARVOX table (Psvr2TriggerPolicy.h
// validTriggerCommand): Weapon start 2-7, end>start, end<=8, strength 1-8;
// Vibration position<=9, amplitude 1-8, frequency>=1.
//
// Pure logic, no I/O, no production dependency beyond psvr2_trigger.h.
// The main agent wires it at the FireAmmunition seam; see docs/TRIGGER_SHOT.md.

#include "psvr2_trigger.h"

namespace triggershot {

using psvr2trigger::Command;
using psvr2trigger::Effect;
using psvr2trigger::Hand;
using psvr2trigger::OffCommand;
using psvr2trigger::OffReason;
using psvr2trigger::Weapon;

// FP gun-mesh fingerprints (same table as GunAlign in d3d9.cpp).
// Gatling has no fingerprinted mesh yet -> Other until the census fills it.
inline Weapon WeaponFromGunVerts(unsigned gunVerts) {
    switch (gunVerts) {
    case 5799: return Weapon::Pistol;
    case 8297: return Weapon::Shotgun;
    case 11387: return Weapon::Rifle;
    case 11389: return Weapon::Sniper;
    case 11390: return Weapon::Sniper;  // railgun: heavy single shot, sniper wall + kick
    default: return Weapon::Other;
    }
}

struct Kick {
    std::uint8_t position = 0;   // Vibration position 0..9
    std::uint8_t amplitude = 0;  // 1..8, significant by design
    std::uint8_t frequency = 0;  // >=1 Hz
    unsigned durationMs = 0;     // kick window after the shot
};

// Significant per-weapon kick. Defaults need one headset pass to confirm feel,
// but every value is inside the validated hardware ranges.
inline Kick KickFor(Weapon w) {
    switch (w) {
    case Weapon::Pistol:  return {3, 6, 50, 150};
    case Weapon::Shotgun: return {2, 8, 35, 220};
    case Weapon::Rifle:   return {3, 7, 40, 120};
    case Weapon::Sniper:  return {2, 8, 30, 250};
    case Weapon::Tmd:
    case Weapon::Other:
    default:              return {0, 0, 0, 0};  // no kick
    }
}

// Gatling spin-up staged vibration (mirrors KHARVOX chaingun 6/25 -> 8/55).
inline Kick SpinUpKick(unsigned fireHeldMs) {
    if (fireHeldMs < 200) return {2, 6, 25, 0};
    if (fireHeldMs < 450) return {2, 7, 35, 0};
    if (fireHeldMs < 750) return {2, 8, 45, 0};
    return {2, 8, 55, 0};
}

inline Command KickCommand(Hand hand, const Kick& k) {
    Command c;
    if (k.amplitude < 1 || k.frequency < 1) return OffCommand(OffReason::UnsupportedWeapon);
    c.effect = Effect::Vibration;
    c.hand = hand;
    c.position = k.position;
    c.amplitude = k.amplitude;
    c.frequency = k.frequency;
    return c;
}

// Shot state machine. OnShotFired() is called from the FireAmmunition seam
// with the session clock; Select() returns the kick while inside the window,
// otherwise the hold wall the caller passes in (from ProfileFor/Select).
class ShotRunner {
public:
    void OnShotFired(unsigned nowMs, Weapon w) {
        weapon_ = w;
        lastShotMs_ = nowMs;
        haveShot_ = (w != Weapon::Tmd && w != Weapon::Other);
    }
    void Reset() { haveShot_ = false; weapon_ = Weapon::Other; lastShotMs_ = 0; }
    void SetHand(Hand h) { hand_ = h; }

    Command Select(const Command& holdWall, unsigned nowMs) const {
        if (!haveShot_) return holdWall;
        const Kick k = KickFor(weapon_);
        if (k.durationMs == 0) return holdWall;
        const unsigned elapsed = nowMs - lastShotMs_;  // u32 wrap: ~49 days, ignore
        if (elapsed <= k.durationMs) return KickCommand(hand_, k);
        return holdWall;
    }

    bool InKick(unsigned nowMs) const {
        if (!haveShot_) return false;
        const Kick k = KickFor(weapon_);
        return k.durationMs != 0 && (nowMs - lastShotMs_) <= k.durationMs;
    }

private:
    Weapon weapon_ = Weapon::Other;
    Hand hand_ = Hand::Right;
    unsigned lastShotMs_ = 0;
    bool haveShot_ = false;
};

// KHARVOX-range validation for anything this header emits.
inline bool ValidKickCommand(const Command& c) {
    if (c.effect == Effect::Off) return true;
    if (c.effect != Effect::Vibration) return false;
    return c.position <= 9 && c.amplitude >= 1 && c.amplitude <= 8 && c.frequency >= 1;
}

}  // namespace triggershot
