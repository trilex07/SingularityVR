# M02 UI Remaster & Sense Prompt Handoff (SIDE UI)

**Date:** 2026-10-24  
**Role:** SIDE UI Specialist  
**Status Baseline:** PARTIAL (Physical PSVR2 test confirms menu navigation works and new 4-gun icons are live, but visual remaster is incomplete and legacy "Pad 11" controller prompts are still visible).

---

## 1. Context & PSVR2 Physical Feedback

During PSVR2 acceptance testing of build `A82184B2`:
- **Functional Pass:** Menu navigation, options selection, Polish dubbing, and weapon wheel mechanics work without crash.
- **Visual Gaps:**
  1. Main menu and pause menu still display legacy low-resolution background elements and plates.
  2. Legacy Xbox/UE3 controller prompts such as **"Pad 11"** appear instead of native VR controller buttons.
  3. Only the 4 primary weapons (Centurion, Valkyrie, Volk, Kasimov) currently have high-detail wheel icons; remaining items/abilities still use legacy sprites.

---

## 2. Verified Active Installation State

The current Steam installation (`C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries`) is verified with:

### A. Configuration Flags (`SingularityVR.ini`)
```ini
[Render]
UiRemasterRuntime=1
UiRemasterSettings=1
UiRemasterWheel=1
VrPromptTokens=1
```

### B. Installed Atlas Checksums
- `vr-assets/ui/remaster/ui_atlas_neutral.png`:
  `3EF1782696B4A5E1E9D93F409C5B0F6FF014B78CC42FAB06EC29110661981818` (**Active: Detailed 4-gun icons**)
- `vr-assets/ui/remaster/ui_atlas_localized_pl.png`:
  `6436AA257F310F65A1F65C04D7435D9BF8343BFEFAB2EEE50A8A836F96FC83FD`
- `vr-assets/ui/remaster/ui_atlas_localized_en.png`:
  `53B570C4787B834270CBCF0943622877613A7EE7E2AD07FA6A363DFE11A422BA`

---

## 3. Scope of Work for SIDE UI Agent

### Task A: Main Menu & Pause Menu Visual Modernization
- Replace legacy textured menu backgrounds and bordered plates with clean high-resolution VR presentation plates (consistent with `ui_atlas_neutral.png` styling).
- Maintain menu positioning, depth plane, and interaction ray hitboxes without altering underlying GFx action bindings.

### Task B: Remaining Icons Remaster
- Prepare high-detail vector/illustrated sprites for:
  - TMD abilities (Impulse, Chrono/Age, Gravity/Telekinesis).
  - Special weapons (Seeker Rifle, Grenade Launcher / Autocannon if equipped).
  - Tactical items, medical packs, and notebook indicators.
- Package into the neutral atlas preserving exact UV slots and metadata manifest.

### Task C: Selective Replacement of "Pad 11" and Raw Controller Tokens
- **Problem:** ActionScript / Scaleform in certain tutorial prompts and dialogs injects raw DirectInput/XInput identifiers like `"Pad 11"`.
- **Solution:**
  - Intercept the text and draw calls for these specific button prompt batches.
  - Map `"Pad 11"` (and any remaining raw Xbox tokens) to explicit PS VR2 Sense controller icons (Cross, Circle, Square, Triangle, L1, R1, L2, R2, Left Grip, Right Grip).
  - Use `prompttokens::Rewrite` and the D3D9 context prompt detector (`LooksLikeContextPrompt` / `DrawVrContextPrompt`).

### Task D: Strict Invariants & Non-Regression Rules
> [!CRITICAL]
> **Subtitles, Objectives & Dubbing Must Remain Intact:**
> - NEVER globally hide GFx layers (`ContextCursorSuppress=1` or zeroing alpha on the whole HUD).
> - Story subtitles (`Subtitles`), mission objectives (`Objectives`), interaction prompts (`Interact / Pickup`), and notebook dialogs must remain 100% visible and timed with Polish dubbing.
> - Do not modify weapon handling, Seeker holster code, or reload dispatch.

---

## 4. Verification & Handoff Checklist for SIDE UI

1. **Offline Regression:**
   - `python asset-tools/test_detailed_weapon_icons.py` PASS
   - `python asset-tools/test_ui_atlas_qa.py` PASS
   - `/O2` build check with `test-ui-remaster-runtime.exe` and `test-ui-remaster-d3d9.exe` PASS
2. **Deliverables:**
   - Updated atlas files and generator script (`asset-tools/build_detailed_weapon_icons.py` / manifest).
   - Targeted C++ patch for prompt token translation if required.
   - Clean git branch for MAIN review and guarded deployment.
