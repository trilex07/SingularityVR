#pragma once
#include <cstdint>
#include <cstring>

// Contextual native prompt state (isolated helper, no production dependency).
//
// Reverse-engineering status (branch feature/psvr2-polish-parallel):
// CONFIRMED:
//   - There are NO GFx-logic hooks in production (no GFxValue/Invoke/
//     SetVariable/SetText/ActionScript/StringAliasMap interception). GPU
//     draws carry glyph quads, never strings.
//   - Closest feed point: HudStereoPair() (spikes/view_matrix/d3d9.cpp)
//     observes per-draw (primCount, stride, clip x/y, scale, texture,
//     c5-c8, on-scene, eye). Centred gameplay prompts measure as
//     stride==20, 16-24 prims, y≈-0.138, orthographic, on-scene.
//   - Runner-up: Hook_ProcessEvent sees script entry names only; no prompt
//     setter (ShowPrompt-shaped UFunction) has been observed. The
//     [token]->button resolver is exe-side and uninstrumented.
// NOT available at the D3D9 level: semantic action, button alias, world
// source id. Those fields exist here as explicitly-UNVERIFIED slots the
// production agent fills ONLY when an exe-level hook lands. Until then the
// helper reports visibility + position and NEVER invents an icon.
//
// Contract (matches the overlay requirement — no permanent legend):
//   hidden            -> no icon state (ShowIcon()==false)
//   prompt shown      -> visible + position; icon only if action verified
//   Interact->Reload  -> state changes (generation bump, new action)
//   hide              -> invisible immediately
//   re-show           -> works (latch re-arms)
//   unknown alias     -> text may stay visible; icon stays off
//   no prompt         -> never requests an icon

namespace ctxprompt {

enum class Action : std::uint8_t {
    Unknown = 0,
    Interact, // "use" family: open / take / talk (verify per template)
    Reload,
    // Extend ONLY with exe-confirmed mappings; never guess from glyphs.
};

struct State {
    bool visible = false;
    Action action = Action::Unknown;
    bool actionVerified = false; // false -> ShowIcon() stays false
    float x = 0; // native clip-space prompt anchor
    float y = 0;
    std::uint64_t sourceId = 0; // 0 = unknown (no object association at D3D9)
    std::uint64_t generation = 0; // bumps on every shown/changed/hidden edge
};

inline const char* ActionName(Action a) {
    switch (a) {
    case Action::Interact: return "interact";
    case Action::Reload: return "reload";
    default: return "unknown";
    }
}

// Icon policy: an icon is requested ONLY for a visible prompt whose action
// was verified through an exe-level (non-glyph) channel.
inline bool ShowIcon(const State& s) { return s.visible && s.actionVerified; }

struct Config {
    unsigned showFrames = 3; // consecutive present-frames to latch shown
    unsigned hideFrames = 2; // consecutive absent-frames to latch hidden
};

class Tracker {
public:
    explicit Tracker(Config cfg = Config{}) : cfg_(cfg) {}
    void Reset() {
        state_ = State{};
        present_ = 0;
        absent_ = 0;
    }
    const State& Get() const { return state_; }
    // Production feed: called once per frame (or present) with the
    // signature-latch result. observed=true means the centred-prompt draw
    // signature (stride 20, 16-24 prims, y≈-0.138, ortho, on-scene) was
    // present; x/y carry its clip anchor. unverifiedAction is recorded but
    // NEVER enables the icon (actionVerified stays false).
    void Observe(bool observed, float x, float y, Action unverifiedAction) {
        if (observed) {
            absent_ = 0;
            if (!state_.visible) {
                if (++present_ >= cfg_.showFrames) {
                    state_.visible = true;
                    state_.x = x;
                    state_.y = y;
                    state_.action = unverifiedAction;
                    state_.actionVerified = false;
                    ++state_.generation;
                    present_ = 0;
                }
            } else {
                // Visible: anchor follows; action change bumps generation
                // but still unverified (no icon) until confirmed.
                state_.x = x;
                state_.y = y;
                if (unverifiedAction != state_.action) {
                    state_.action = unverifiedAction;
                    state_.actionVerified = false;
                    ++state_.generation;
                }
                present_ = 0;
            }
        } else {
            present_ = 0;
            if (state_.visible) {
                if (++absent_ >= cfg_.hideFrames) {
                    const std::uint64_t gen = state_.generation + 1;
                    state_ = State{};
                    state_.generation = gen;
                    absent_ = 0;
                }
            }
        }
    }
    // Exe-level confirmation path (future): marks the CURRENT visible
    // prompt's action verified, enabling its icon. No-op when hidden or
    // when the action disagrees (never verify a stale prompt).
    void ConfirmAction(Action verified) {
        if (state_.visible && verified != Action::Unknown && verified == state_.action) {
            state_.actionVerified = true;
            ++state_.generation;
        }
    }
    void OnHidden() {
        if (state_.visible) {
            const std::uint64_t gen = state_.generation + 1;
            state_ = State{};
            state_.generation = gen;
        }
        present_ = 0;
        absent_ = 0;
    }
    void SetShowFrames(unsigned n) { if (n > 0) cfg_.showFrames = n; }
    void SetHideFrames(unsigned n) { if (n > 0) cfg_.hideFrames = n; }

private:
    Config cfg_;
    State state_;
    unsigned present_ = 0;
    unsigned absent_ = 0;
};

} // namespace ctxprompt
