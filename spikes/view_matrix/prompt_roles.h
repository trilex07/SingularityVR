#pragma once

// Maps a game/native action to a PlayStation prompt icon name. The names match
// assets/processed/ui/psvr2-input-icons.json and the generated
// spikes/view_matrix/prompt_atlas_generated.h. Grounded in the verified native
// aliases in docs/PSVR2_PROMPT_MAP.md (CB_FIRE/AIM/JUMP/CROUCH/USE/CYCLEWEAPON/
// AGEYOUNG/IMPULSE/DASH/ANTIGRAVITY/FLASHLIGHT/HEALTH) and the user's request
// (Enter -> Cross, interact -> R1, etc.). Pure logic, no dependency.

namespace promptroles {

enum class Action {
    Confirm,      // Enter / menu select / A (jump-confirm)  -> Cross
    Back,         // Esc / B                                  -> Circle
    Fire,         // RT (CB_FIRE)                             -> R2
    Aim,          // LT (CB_AIM)                              -> L2
    Interact,     // use/interact                             -> R1
    Jump,         // A (CB_JUMP)                              -> Cross
    Crouch,       // B (CB_CROUCH)                            -> Circle
    Age,          // X (CB_AGEYOUNG)                          -> Square
    CycleWeapon,  // Y (CB_CYCLEWEAPON)                       -> Triangle
    Impulse,      // RB (CB_IMPULSE)                          -> L1
    Dash,         // L-click (CB_DASH)                        -> L3
    AntiGrav,     // R-click (CB_ANTIGRAVITY)                 -> R3
    Flashlight,   // D-Down (CB_FLASHLIGHT)                   -> DpadDown
    Health,       // D-Up (CB_HEALTH)                         -> DpadUp
    Pause,        // Start (ShowMenu)                         -> Options
    Create,       // Back/Scoreboard (ToggleScoreboard)       -> Create
    Move,         // left stick                               -> StickL
    Look,         // right stick                              -> StickR
    Touchpad,     // touchpad                                 -> Touchpad
    Unknown,
};

inline const char* IconName(Action a) {
    switch (a) {
    case Action::Confirm:     return "Cross";
    case Action::Back:        return "Circle";
    case Action::Fire:        return "R2";
    case Action::Aim:         return "L2";
    case Action::Interact:    return "R1";
    case Action::Jump:        return "Cross";
    case Action::Crouch:      return "Circle";
    case Action::Age:         return "Square";
    case Action::CycleWeapon: return "Triangle";
    case Action::Impulse:     return "L1";
    case Action::Dash:        return "L3";
    case Action::AntiGrav:    return "R3";
    case Action::Flashlight:  return "DpadDown";
    case Action::Health:      return "DpadUp";
    case Action::Pause:       return "Options";
    case Action::Create:      return "Create";
    case Action::Move:        return "StickL";
    case Action::Look:        return "StickR";
    case Action::Touchpad:    return "Touchpad";
    case Action::Unknown:
    default:                  return "Unknown";
    }
}

} // namespace promptroles
