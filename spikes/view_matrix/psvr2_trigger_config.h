#pragma once
#include "psvr2_trigger.h"
#include <cctype>
#include <cstdint>
#include <cstring>

// [PSVR2] INI configuration for adaptive-trigger profiles (clean-room).
//
// Concept (research brief section 18):
//   [PSVR2]
//   AdaptiveTriggers=1
//   TriggerPistol=4,6,3
//   TriggerRifle=3,5,5
//   TriggerRifleFire=3,7,40
//   TriggerShotgun=2,4,7
//   TriggerSniper=2,5,6
// Triples are KHARVOX-convention (start,end,strength); the fire override is
// (position,amplitude,frequency). Defaults and clamp ranges mirror the proven
// KHARVOX table (Psvr2TriggerPolicy.h:188-253: pistol weapon(4,6,3); rifle
// weapon(3,5,5) + fire vibration(3,7,40); shotgun weapon(2,4,7)) and its
// validation ranges (validTriggerCommand: Weapon start 2-7, end>start,
// end<=8, strength 1-8; Vibration position<=9, amplitude 1-8, frequency>=1).
// SNIPER and TMD have no KHARVOX equivalent: sniper ships a provisional heavy
// wall (NEEDS HEADSET VALIDATION), TMD stays Off until profiled.
//
// Pure parser: feeds on text, no file I/O, no d3d9.cpp dependency. Per-key
// fallback (one bad line never wipes the other profiles).

namespace psvr2config {

struct Triple {
    std::uint8_t a = 0, b = 0, c = 0;
};

struct Profiles {
    bool enabled = true;
    Triple pistol{4, 6, 3};
    Triple rifle{3, 5, 5};
    Triple rifleFire{3, 7, 40};
    Triple shotgun{2, 4, 7};
    Triple sniper{2, 5, 6}; // provisional; needs headset validation
};

inline bool ClampWeaponTriple(Triple& t) {
    if (t.a < 1 || t.a > 7) return false;
    if (t.b <= t.a || t.b > 8) return false;
    if (t.c < 1 || t.c > 8) return false;
    return true;
}
inline bool ClampVibrationTriple(Triple& t) {
    if (t.a > 9 || t.c < 1) return false;
    if (t.b < 1 || t.b > 8) return false;
    return true;
}

inline bool ParseTriple(const char* s, Triple& out) {
    if (!s) return false;
    while (*s == ' ' || *s == '\t') ++s;
    char* end = nullptr;
    const long a = std::strtol(s, &end, 10);
    if (end == s || *end != ',') return false;
    s = end + 1;
    const long b = std::strtol(s, &end, 10);
    if (end == s || *end != ',') return false;
    s = end + 1;
    const long c = std::strtol(s, &end, 10);
    if (end == s || a < 0 || a > 255 || b < 0 || b > 255 || c < 0 || c > 255) return false;
    while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n') ++end;
    if (*end != '\0') return false;
    out.a = (std::uint8_t)a;
    out.b = (std::uint8_t)b;
    out.c = (std::uint8_t)c;
    return true;
}

inline bool KeyEquals(const char* line, std::size_t n, const char* key) {
    for (std::size_t i = 0;; ++i) {
        char lc = (char)std::tolower((unsigned char)line[i]);
        if (key[i] == '\0') return i == n || (i < n && (line[i] == ' ' || line[i] == '\t'));
        if (i >= n || lc != key[i]) return false;
    }
}

// Parses the [PSVR2] section of INI text. Unknown keys ignored; invalid
// values leave the corresponding default in place. Returns keys accepted.
inline unsigned ParseIni(const char* text, Profiles& p) {
    if (!text) return 0;
    unsigned accepted = 0;
    bool inSection = false;
    const char* line = text;
    char buf[256];
    while (*line) {
        std::size_t n = 0;
        while (line[n] && line[n] != '\n' && n + 1 < sizeof(buf)) {
            buf[n] = line[n];
            ++n;
        }
        while (line[n] && line[n] != '\n') ++n; // overlong line: skip rest
        buf[n] = '\0';
        line += n;
        if (*line == '\n') ++line;
        // trim leading space; skip blanks/comments
        std::size_t s = 0;
        while (buf[s] == ' ' || buf[s] == '\t' || buf[s] == '\r') ++s;
        if (buf[s] == '\0' || buf[s] == ';' || buf[s] == '#') continue;
        if (buf[s] == '[') {
            inSection = KeyEquals(buf + s + 1, n > s + 1 ? n - s - 1 : 0, "psvr2]");
            continue;
        }
        if (!inSection) continue;
        const char* eq = std::strchr(buf + s, '=');
        if (!eq) continue;
        // key length trimmed of trailing space
        const char* ke = eq;
        while (ke > buf + (int)s && (ke[-1] == ' ' || ke[-1] == '\t')) --ke;
        const std::size_t kn = (std::size_t)(ke - (buf + s));
        const char* val = eq + 1;
        if (KeyEquals(buf + s, kn, "adaptivetriggers")) {
            long v = std::strtol(val, nullptr, 10);
            p.enabled = v != 0;
            ++accepted;
        } else if (KeyEquals(buf + s, kn, "triggerpistol")) {
            Triple t;
            if (ParseTriple(val, t) && ClampWeaponTriple(t)) {
                p.pistol = t;
                ++accepted;
            }
        } else if (KeyEquals(buf + s, kn, "triggerriflefire")) {
            Triple t;
            if (ParseTriple(val, t) && ClampVibrationTriple(t)) {
                p.rifleFire = t;
                ++accepted;
            }
        } else if (KeyEquals(buf + s, kn, "triggerrifle")) {
            Triple t;
            if (ParseTriple(val, t) && ClampWeaponTriple(t)) {
                p.rifle = t;
                ++accepted;
            }
        } else if (KeyEquals(buf + s, kn, "triggershotgun")) {
            Triple t;
            if (ParseTriple(val, t) && ClampWeaponTriple(t)) {
                p.shotgun = t;
                ++accepted;
            }
        } else if (KeyEquals(buf + s, kn, "triggersniper")) {
            Triple t;
            if (ParseTriple(val, t) && ClampWeaponTriple(t)) {
                p.sniper = t;
                ++accepted;
            }
        }
    }
    return accepted;
}

// Config-driven profile selection: same shape as psvr2trigger::ProfileFor
// but sourced from Profiles (TMD/OTHER stay Off; rifle fire override kept).
inline psvr2trigger::Command ProfileFromConfig(psvr2trigger::Weapon weapon, bool fireDown,
                                               const Profiles& p) {
    using namespace psvr2trigger;
    Command c;
    c.hand = Hand::Right;
    switch (weapon) {
    case Weapon::Pistol:
        c.effect = Effect::Weapon;
        c.start = p.pistol.a;
        c.end = p.pistol.b;
        c.strength = p.pistol.c;
        return c;
    case Weapon::Rifle:
        if (fireDown) {
            c.effect = Effect::Vibration;
            c.position = p.rifleFire.a;
            c.amplitude = p.rifleFire.b;
            c.frequency = p.rifleFire.c;
            return c;
        }
        c.effect = Effect::Weapon;
        c.start = p.rifle.a;
        c.end = p.rifle.b;
        c.strength = p.rifle.c;
        return c;
    case Weapon::Shotgun:
        c.effect = Effect::Weapon;
        c.start = p.shotgun.a;
        c.end = p.shotgun.b;
        c.strength = p.shotgun.c;
        return c;
    case Weapon::Sniper:
        c.effect = Effect::Weapon;
        c.start = p.sniper.a;
        c.end = p.sniper.b;
        c.strength = p.sniper.c;
        return c;
    case Weapon::Tmd:
    case Weapon::Other:
        return OffCommand(OffReason::UnsupportedWeapon);
    }
    return OffCommand(OffReason::UnsupportedWeapon);
}

// Config-driven policy: identical gating to psvr2trigger::Select, but the
// idle/fire profiles come from Profiles (main agent binds the parsed INI).
// ProfileFromConfig already scopes the fire override to rifles, matching
// Select's semantics exactly.
inline psvr2trigger::Command SelectWithProfiles(const psvr2trigger::PolicyInput& in,
                                                const Profiles& p) {
    using namespace psvr2trigger;
    if (!in.backendAvailable) return OffCommand(OffReason::Disabled);
    if (!in.runtimeCompatible) return OffCommand(OffReason::UnsupportedRuntime);
    if (!in.sessionFocused) return OffCommand(OffReason::Unfocused);
    if (!in.gameplayActive) return OffCommand(in.inactiveReason);
    if (!p.enabled) return OffCommand(OffReason::Disabled);
    return ProfileFromConfig(in.weapon, in.fireDown && in.ammoOk, p);
}

} // namespace psvr2config
