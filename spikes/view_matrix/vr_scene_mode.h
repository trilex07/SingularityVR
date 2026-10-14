#pragma once
#include <cstdint>

namespace vrscene {

enum class Mode : std::uint8_t {
    GameplayStereo,
    CutsceneFlat,
    VideoFlat,
    MenuFlat,
    Unknown,
};

struct Signals {
    bool cinematic = false;   // UE3 SetCinematicMode state
    bool video = false;       // confirmed Bink/full-frame draw in this frame
    bool menu = false;        // native GFx menu focus; classification only
    bool paused = false;
    bool frontendMenu = false; // opt-in identified native main menu; not dialogue/pause
    bool cinemaPauseMenu = false; // explicit focused pause owns presentation, not raw cinema
};

inline bool FrontendMenuPresentation(bool enabled, int menuClass, bool focus) {
    return enabled && focus && menuClass == 2;
}

struct Transition {
    Mode from = Mode::Unknown;
    Mode to = Mode::Unknown;
    bool changed = false;
    std::uint32_t reason = 0;
};

// Small, frame-driven hysteresis controller. It deliberately requires a stable
// cinematic observation before entering and a longer clear run before leaving.
// Video is an explicit draw observation and enters immediately; menu never
// requests mono rendering because its renderer owns its own flat presentation.
class Controller {
public:
    static constexpr std::uint32_t kEnterFrames = 2;
    static constexpr std::uint32_t kExitFrames = 4;

    Mode Current() const { return mode_; }
    bool Flat() const { return mode_ == Mode::CutsceneFlat || mode_ == Mode::VideoFlat; }
    // A menu/loading/video source must be presented as the same image in both
    // SBS eye halves. This remains distinct from Flat(): offscreen UI targets
    // still have to be rendered exactly once.
    bool NeedsMonoPair() const {
        // Bink/intro uses DrawVideoQuadPerEye; MonoPair on the scene RT here
        // stacks with that and reads as doubled/cropped video in the headset.
        return mode_ == Mode::CutsceneFlat || mode_ == Mode::MenuFlat;
    }
    std::uint32_t StableFrames() const { return stable_; }

    Transition Advance(const Signals& s) {
        Transition out{mode_, mode_, false, 0};
        if (s.cinemaPauseMenu) {
            cinematicFrames_ = 0; clearFrames_ = 0;
            return Change(Mode::MenuFlat, 32u, out);
        }
        if (s.video) {
            cinematicFrames_ = 0; clearFrames_ = 0;
            return Change(Mode::VideoFlat, 2u, out);
        }
        if (s.frontendMenu) {
            // Native frontend backdrops may keep the cinematic bit ON.
            // The frontend owns its accepted per-eye UI route, not full-SBS
            // cinema fitting. Do not rewrite the native cinematic flag.
            cinematicFrames_ = 0; clearFrames_ = 0;
            return Change(Mode::MenuFlat, 16u, out);
        }
        if (s.cinematic) {
            ++cinematicFrames_; clearFrames_ = 0;
            if (cinematicFrames_ >= kEnterFrames)
                return Change(Mode::CutsceneFlat, 1u, out);
            return out;
        }
        cinematicFrames_ = 0;
        if (mode_ == Mode::CutsceneFlat || mode_ == Mode::VideoFlat) {
            ++clearFrames_;
            if (clearFrames_ < kExitFrames) return out;
            clearFrames_ = 0;
        }
        const Mode next = s.menu ? Mode::MenuFlat : Mode::GameplayStereo;
        return Change(next, s.menu ? 4u : (s.paused ? 8u : 0u), out);
    }

private:
    Mode mode_ = Mode::GameplayStereo;
    std::uint32_t cinematicFrames_ = 0;
    std::uint32_t clearFrames_ = 0;
    std::uint32_t stable_ = 0;

    Transition Change(Mode next, std::uint32_t reason, Transition out) {
        if (next == mode_) { ++stable_; return out; }
        mode_ = next; stable_ = 1;
        out.to = next; out.changed = true; out.reason = reason;
        return out;
    }
};

inline const char* Name(Mode mode) {
    switch (mode) {
    case Mode::GameplayStereo: return "GameplayStereo";
    case Mode::CutsceneFlat: return "CutsceneFlat";
    case Mode::VideoFlat: return "VideoFlat";
    case Mode::MenuFlat: return "MenuFlat";
    default: return "Unknown";
    }
}

} // namespace vrscene
