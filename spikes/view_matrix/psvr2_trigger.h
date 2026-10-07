#pragma once
#include <cmath>
#include <cstdint>

// PSVR2 Sense adaptive-trigger abstraction for SingularityVR.
//
// Attribution: weapon parameter ranges are derived from the behaviour of
// KHARVOX (MIT License, Copyright (c) 2026 CactusVRStudios) at commit
// ff348bbd62204067fe6e16abc99be2526cd08dc0, file
// src/psvr2/Psvr2TriggerPolicy.h, function selectTriggerCommand(). See
// docs/REFERENCE_VR_IMPLEMENTATIONS.md for the full audit trail. This header
// is an original clean-room abstraction -- it contains no copied KHARVOX or
// PSVR2Toolkit code and includes no PSVR2Toolkit CAPI headers.
//
// Architecture (from KHARVOX src/psvr2/Psvr2IpcClient.cpp,
// Psvr2BridgeMain.cpp, launcher/KharvoxLauncher/Psvr2BridgeSession.cs):
//   - The 32-bit game process must NEVER LoadLibrary the PSVR2Toolkit DLL
//     (x64-only PE, third-party/psvr2-toolkit/psvr2_toolkit_capi_loader.dll
//     is Machine 0x8664). Hardware access lives in a separate x64 helper
//     process reached over a write-only named pipe; the game only submits
//     small TriggerState messages and dedupes them (packed-uint64 compare,
//     forward-on-change, 50 ms worker poll).
//   - IBackend below is that seam: a future x64 bridge implements Send().
//     With no backend, Runner degrades to a null sink and OpenXR gameplay
//     continues unchanged.
//
// Safety rules (same source):
//   - Init once, persistent handles, never enumerate on the hot path.
//   - Gameplay event updates cached requested state; hardware update happens
//     on the backend's own low-overhead path.
//   - Any backend failure -> Off command, gameplay continues.
//   - Target RIGHT Sense trigger initially; Hand is carried so a future
//     left-hand pass needs no protocol change.

namespace psvr2trigger {

enum class Weapon : std::uint8_t { Pistol, Rifle, Shotgun, Sniper, Tmd, Other };
enum class Effect : std::uint8_t { Off, Weapon, Vibration, Feedback, SlopeFeedback };
enum class Hand : std::uint8_t { Right = 0, Left = 1 };
enum class OffReason : std::uint8_t {
    None, Disabled, UnsupportedRuntime, Unfocused, Inactive, SessionEnd, UnsupportedWeapon
};

struct Command {
    Effect effect = Effect::Off;
    Hand hand = Hand::Right;
    // Weapon mode: start/end travel positions (2..8, end > start), strength 1..8.
    std::uint8_t start = 0, end = 0, strength = 0;
    // Vibration mode: actuator position 0..9, amplitude 1..8, frequency >= 1.
    std::uint8_t position = 0, amplitude = 0, frequency = 0;
    // SlopeFeedback mode: start < end <= 9, strengths 1..8.
    std::uint8_t startStrength = 0, endStrength = 0;
    OffReason reason = OffReason::None;
};

inline Command OffCommand(OffReason reason) {
    Command c;
    c.effect = Effect::Off;
    c.reason = reason;
    return c;
}

// Provisional weapon profiles. PISTOL/RIFLE/SHOTGUN ranges mirror KHARVOX's
// proven table (pistol weapon(4,6,3); rifle weapon(3,5,5) + auto-fire
// vibration(3,7,40); shotgun weapon(2,4,7)). SNIPER has no KHARVOX equivalent;
// the deliberate heavy-wall starting point below NEEDS HEADSET VALIDATION
// before any gameplay wiring. TMD/OTHER stay Off until profiled.
inline Command ProfileFor(Weapon weapon, bool fireDown) {
    Command c;
    c.hand = Hand::Right;
    switch (weapon) {
    case Weapon::Pistol:
        c.effect = Effect::Weapon;
        c.start = 4; c.end = 6; c.strength = 3;
        return c;
    case Weapon::Rifle:
        if (fireDown) {
            c.effect = Effect::Vibration;
            c.position = 3; c.amplitude = 7; c.frequency = 40;
            return c;
        }
        c.effect = Effect::Weapon;
        c.start = 3; c.end = 5; c.strength = 5;
        return c;
    case Weapon::Shotgun:
        c.effect = Effect::Weapon;
        c.start = 2; c.end = 4; c.strength = 7;
        return c;
    case Weapon::Sniper:
        c.effect = Effect::Weapon;
        c.start = 2; c.end = 5; c.strength = 6;
        return c;
    case Weapon::Tmd:
    case Weapon::Other:
        return OffCommand(OffReason::UnsupportedWeapon);
    }
    return OffCommand(OffReason::UnsupportedWeapon);
}

struct PolicyInput {
    bool backendAvailable = false; // launcher preflight: bridge + loader present
    bool runtimeCompatible = false; // SteamVR-backed OpenXR runtime only
    bool sessionFocused = false; // OpenXR session focused
    bool gameplayActive = false; // camera + world active, no menu/loading/death/cinematic
    OffReason inactiveReason = OffReason::Inactive;
    Weapon weapon = Weapon::Other;
    bool fireDown = false;
    bool ammoOk = true; // Empty/Unavailable blocks fire vibration only
};

// Mirrors KHARVOX selectTriggerCommand gating order:
// Disabled -> UnsupportedRuntime -> Unfocused -> !gameplay -> fire override
// (ammo-gated) -> idle profile.
inline Command Select(const PolicyInput& in) {
    if (!in.backendAvailable) return OffCommand(OffReason::Disabled);
    if (!in.runtimeCompatible) return OffCommand(OffReason::UnsupportedRuntime);
    if (!in.sessionFocused) return OffCommand(OffReason::Unfocused);
    if (!in.gameplayActive) return OffCommand(in.inactiveReason);
    if (in.fireDown && in.ammoOk && in.weapon == Weapon::Rifle)
        return ProfileFor(Weapon::Rifle, true);
    return ProfileFor(in.weapon, false);
}

// Packed-uint64 dedup codec: forward to the backend only on change, exactly
// like KHARVOX packTriggerCommand/unpackTriggerCommand + generation counter.
// Tight 43-bit layout (all fields masked): effect 3 | hand 1 | start 4 |
// end 4 | strength 4 | position 4 | amplitude 4 | frequency 7 |
// startStrength 4 | endStrength 4 | reason 4. Values must respect the
// KHARVOX validation ranges (start 2-7, end<=8, strengths 1-8, position<=9,
// amplitude 1-8); out-of-range values collide by design — validate at the
// profile layer (psvr2_trigger_config.h Clamp*), never here.
inline std::uint64_t Pack(const Command& c) {
    std::uint64_t v = (std::uint64_t)((unsigned)c.effect & 0x7u);
    v = (v << 1) | (std::uint64_t)((unsigned)c.hand & 0x1u);
    v = (v << 4) | (c.start & 0xFu);
    v = (v << 4) | (c.end & 0xFu);
    v = (v << 4) | (c.strength & 0xFu);
    v = (v << 4) | (c.position & 0xFu);
    v = (v << 4) | (c.amplitude & 0xFu);
    v = (v << 7) | (c.frequency & 0x7Fu);
    v = (v << 4) | (c.startStrength & 0xFu);
    v = (v << 4) | (c.endStrength & 0xFu);
    v = (v << 4) | ((std::uint64_t)((unsigned)c.reason & 0xFu));
    return v;
}

struct IBackend {
    virtual ~IBackend() {}
    // Called at most once per policy change (not per frame). Must be
    // non-blocking; failures are swallowed by Runner. Never called with
    // identical consecutive commands.
    virtual bool Send(const Command& command) = 0;
};

struct NullBackend : IBackend {
    bool Send(const Command&) override { return false; }
};

// Game-event entry points (section 27 of the research brief):
//   FirePressed/FireReleased -> SetFireDown(bool)
//   WeaponChanged            -> SetWeapon(...)
//   ReloadStarted/Finished   -> SetReloading(bool) (suppresses fire vibration)
// The runner owns the cached requested state; Update() evaluates Select()
// and forwards only on change. Backend absence/failure never throws.
class Runner {
public:
    explicit Runner(IBackend* backend) : backend_(backend ? backend : &null_) {}

    void SetWeapon(Weapon w) {
        if (w != weapon_) { weapon_ = w; dirty_ = true; }
    }
    void SetFireDown(bool down) {
        if (down != fireDown_) { fireDown_ = down; dirty_ = true; }
    }
    void SetReloading(bool reloading) {
        if (reloading != reloading_) { reloading_ = reloading; dirty_ = true; }
    }
    void SetGating(bool backendAvailable, bool runtimeCompatible,
                   bool sessionFocused, bool gameplayActive,
                   OffReason inactive = OffReason::Inactive) {
        backendAvailable_ = backendAvailable;
        runtimeCompatible_ = runtimeCompatible;
        sessionFocused_ = sessionFocused;
        gameplayActive_ = gameplayActive;
        inactiveReason_ = inactive;
        dirty_ = true;
    }

    // Returns true when a command was forwarded to the backend this call.
    bool Update() {
        PolicyInput in;
        in.backendAvailable = backendAvailable_;
        in.runtimeCompatible = runtimeCompatible_;
        in.sessionFocused = sessionFocused_;
        in.gameplayActive = gameplayActive_;
        in.inactiveReason = inactiveReason_;
        in.weapon = weapon_;
        in.fireDown = fireDown_ && !reloading_;
        in.ammoOk = ammoOk_;
        Command cmd = Select(in);
        const std::uint64_t packed = Pack(cmd);
        if (!dirty_ && packed == lastSent_) return false;
        dirty_ = false;
        lastSent_ = packed;
        last_ = cmd;
        if (backend_) backend_->Send(cmd); // failure is non-fatal by contract
        ++sends_;
        return true;
    }

    void SetAmmoOk(bool ok) {
        if (ok != ammoOk_) { ammoOk_ = ok; dirty_ = true; }
    }
    // Gameplay event interface (research brief section 17): thin aliases so
    // the main agent can connect weapon/fire/reload hooks without touching
    // Runner internals. No behaviour change versus the Set* methods.
    void OnWeaponChanged(Weapon w) { SetWeapon(w); }
    void OnFirePressed() { SetFireDown(true); }
    void OnFireReleased() { SetFireDown(false); }
    void OnReloadStarted() { SetReloading(true); }
    void OnReloadFinished() { SetReloading(false); }
    bool ClearTriggerEffects() { return ClearTrigger(); }
    const Command& Last() const { return last_; }
    unsigned Sends() const { return sends_; }

    // Session teardown / focus loss path: force Off(SessionEnd) once.
    bool ClearTrigger() {
        Command cmd = OffCommand(OffReason::SessionEnd);
        const std::uint64_t packed = Pack(cmd);
        if (packed == lastSent_ && !dirty_) return false;
        dirty_ = false;
        lastSent_ = packed;
        last_ = cmd;
        if (backend_) backend_->Send(cmd);
        ++sends_;
        return true;
    }

private:
    IBackend* backend_;
    NullBackend null_;
    Weapon weapon_ = Weapon::Other;
    bool fireDown_ = false, reloading_ = false, ammoOk_ = true;
    bool backendAvailable_ = false, runtimeCompatible_ = false;
    bool sessionFocused_ = false, gameplayActive_ = false;
    OffReason inactiveReason_ = OffReason::Inactive;
    Command last_;
    std::uint64_t lastSent_ = Pack(OffCommand(OffReason::Disabled)) ^ 0x9e3779b97f4a7c15ull;
    bool dirty_ = true;
    unsigned sends_ = 0;
};

} // namespace psvr2trigger
