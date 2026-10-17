#pragma once
// Scripts-readiness gate: pure logic, no engine state, L1-testable.
// Decides whether the VR script layer is up yet: route-2 trace hook live,
// key gameplay offsets resolved, and player armed state known (not UNKNOWN).
// While not ready, SyncXRInput holds VR locomotion (sticks/turn) so the game
// does not let the player play with half-wired VR. Buttons, menu nav and the
// native game input path are untouched by the hold.
namespace scriptsready {

// Inputs sampled from the existing sentinels (see d3d9.cpp RefreshArmedState).
struct Signals {
    bool routeHooked;   // g_origSLC != null (route-2 aim hook installed)
    bool offsetsOk;     // g_offWeapon >= 0 && g_offHandsStatus >= 0
    bool armedKnown;    // PlayerArmed() != ARMED_UNKNOWN
};

inline bool Ready(const Signals& s) {
    return s.routeHooked && s.offsetsOk && s.armedKnown;
}

// Failsafe bound for the hold, in Present ticks. Past this the game proceeds
// anyway (and logs once) so a stuck resolver can never wedge locomotion.
inline unsigned long HoldTimeoutFrames() { return 1800; }  // ~15 s at 120 fps

// Pure hold decision. enabled = [Input] WaitForScripts. latchedReady sticks
// once Ready() has fired (cleared on camera loss / level transition).
// unreadyFrames counts consecutive Present ticks while !ready.
inline bool HoldActive(bool enabled, bool latchedReady, unsigned long unreadyFrames) {
    if (!enabled || latchedReady) return false;
    return unreadyFrames < HoldTimeoutFrames();
}

} // namespace scriptsready
