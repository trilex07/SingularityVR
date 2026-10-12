#pragma once

// Cutscene detection voter for SingularityVR flat-cinema presentation.
//
// Research-only helper. It is intentionally independent of d3d9.cpp and of
// vr_scene_mode.h: it qualifies the RAW engine signals into one trustworthy
// cinematic boolean that the existing Present-time controller can consume.
// No weapon, glove, IK, input, or renderer state is touched here.
//
// Lessons encoded (all measured in-repo, see docs/CUTSCENE_DETECTION_20261012.md):
//   - Menus report CINE YES (HISTORY run 176): a lone cinematic flag while a
//     native menu has focus is demoted, never presented flat on its own.
//   - g_inCinematic latched ON permanently (run 211): a latch without a clear
//     path is a bug, so every latched state has three explicit clear sources
//     (exit edge, dead-camera instance change, N consecutive clear frames).
//     A live old camera means a Matinee director cut, never a transition.
//   - Hysteresis is counted in frames, never seconds or per-level timings.

#include <cstdint>

namespace cutscene {

// Raw per-frame observations at the Present seam. The production hook fills
// these from the already existing signals (script hook, video classifier,
// GFx focus, pause state, camera-instance tracker). No new scans.
struct RawSignals {
    bool cinematicHook = false;  // UE3 SetCinematicMode state (g_inCinematic)
    bool cinematicExit = false;  // explicit 0x0 edge observed this frame
    bool videoQuad = false;      // confirmed Bink/full-frame draw this frame
    bool menuFocus = false;      // native GFx menu focus (g_vrUiMenu)
    bool paused = false;         // SetPause state (diagnostic only)
    // Camera-instance tracking (run-211 backstop, refined — see Latch):
    // cameraChanged reports that the tracked instance differs from last
    // frame; oldCameraLive reports whether the PREVIOUS instance still
    // passes the production liveness check (IsLive). A dead old camera is
    // a level transition; a live old camera is a Matinee director cut
    // between camera actors and must NOT clear an engaged cinematic.
    bool cameraChanged = false;
    bool oldCameraLive = false;
};

// Why a vote came out the way it did. Bitmask so the Present-time trace can
// log the exact source set without string formatting on the hot path.
enum Reason : std::uint32_t {
    kNone = 0,
    kHook = 1u << 0,        // cinematic hook asserted
    kVideo = 1u << 1,       // confirmed video quad asserted
    kMenuDemote = 1u << 2,  // hook demoted: menu focus, no video corroboration
    kCameraClear = 1u << 3, // camera-instance change cleared the latch
    kExitEdge = 1u << 4,    // explicit exit edge cleared the latch
    kHysteresis = 1u << 5,  // entry/exit hysteresis gate held the decision
};

struct Vote {
    bool cinematic = false;
    std::uint32_t reasons = kNone;
};

// One-frame qualification: menu-context cinematic without video
// corroboration is the known false positive, so it is demoted here and the
// demotion is reported (not silently dropped) for the trace log.
inline Vote Qualify(const RawSignals& s) {
    Vote v{};
    if (s.videoQuad) {
        v.cinematic = true;
        v.reasons |= kVideo;
    }
    if (s.cinematicHook) {
        v.reasons |= kHook;
        if (s.menuFocus && !s.videoQuad) {
            v.reasons |= kMenuDemote;  // stays non-cinematic on hook alone
        } else {
            v.cinematic = true;
        }
    }
    return v;
}

// Edge-aware latch with a mandatory clear path. Entry needs kEnterFrames
// consecutive qualified-ON frames. Exit is immediate on an explicit exit
// edge, or on a camera-instance change ONLY when the old instance is dead
// (level transition — a live old camera is a Matinee director cut and the
// latch holds through it). Otherwise exit takes kClearFrames consecutive
// qualified-OFF frames. O(1) integer counters: no frame-time cost.
class Latch {
public:
    static constexpr std::uint32_t kEnterFrames = 2;
    static constexpr std::uint32_t kClearFrames = 4;

    bool Engaged() const { return engaged_; }

    // Returns true when engagement changed this frame.
    bool Advance(const Vote& vote, const RawSignals& raw) {
        if (raw.cinematicExit) {
            onFrames_ = 0;
            offFrames_ = 0;
            if (engaged_) {
                engaged_ = false;
                lastClear_ = kExitEdge;
                return true;
            }
            lastClear_ = kExitEdge;
            return false;
        }
        if (raw.cameraChanged) {
            if (!raw.oldCameraLive) {
                onFrames_ = 0;
                offFrames_ = 0;
                if (engaged_) {
                    engaged_ = false;
                    lastClear_ = kCameraClear;
                    return true;
                }
                lastClear_ = kCameraClear;
                return false;
            }
            // Director cut: hold engagement, restart the clear count so a
            // cut cannot shorten the hysteresis window.
            offFrames_ = 0;
        }
        if (vote.cinematic) {
            // Confirmed video is a direct draw observation, not a sampled
            // flag: it engages on the first frame (matches the existing
            // VideoFlat immediate-entry rule). Hook-driven entry keeps the
            // 2-frame hysteresis against single-sample noise.
            if ((vote.reasons & kVideo) != 0) {
                offFrames_ = 0;
                onFrames_ = kEnterFrames;
                if (!engaged_) {
                    engaged_ = true;
                    return true;
                }
                return false;
            }
            ++onFrames_;
            offFrames_ = 0;
            if (!engaged_ && onFrames_ >= kEnterFrames) {
                engaged_ = true;
                return true;
            }
            return false;
        }
        onFrames_ = 0;
        if (engaged_) {
            ++offFrames_;
            if (offFrames_ >= kClearFrames) {
                engaged_ = false;
                offFrames_ = 0;
                lastClear_ = kHysteresis;
                return true;
            }
        }
        return false;
    }

    std::uint32_t LastClearReason() const { return lastClear_; }

private:
    bool engaged_ = false;
    std::uint32_t onFrames_ = 0;
    std::uint32_t offFrames_ = 0;
    std::uint32_t lastClear_ = kNone;
};

}  // namespace cutscene
