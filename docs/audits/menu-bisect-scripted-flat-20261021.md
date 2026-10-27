# Menu bisect: EF3A (OK) → 9AFB (broken)

**Date:** 2026-10-21  
**Evidence:** PSVR2 — `EF3A1F3B` + INI `55A3ED4D` menu OK; `9AFB9225` + INI `DBAB9003` menu broken before any `9C82` sprint patch.

## Commit bisect (offline)

| Step | Artifact | SHA (DLL) | `MainMenuVideoStereo` in binary | Menu (user) |
|------|-----------|-----------|----------------------------------|-------------|
| Accepted | `a53a180` | `EF3A1F3B…` | no | OK |
| Last committed follow-up | **`da690c7`** | **`594C1A47…`** (worktree build) | **no** | *not headset-tested; same menu path as EF3A for attract* |
| Scripted-flat deploy | uncommitted → `9AFB9225` | yes | **broken** |

**No single git commit between `a53a180` and `da690c7` introduces `MainMenuVideoStereo`.**  
All nine commits on `spikes/view_matrix/` in that range are Kathryn/gore/R1/bridge/restraint — **`UpdateSceneModeFrame` at `da690c7` still uses plain `g_inCinematic`**, same shape as accepted.

The menu regression is the **`kathryn-scripted-flat` uncommitted layer** (DLL copied to `build/combined-followups/kathryn-scripted-flat/` + INI prep), not the Kathryn commit chain itself.

## Binary string probe (DLL)

| Marker | EF3A | da690c7 | 9AFB |
|--------|------|---------|------|
| `MainMenuVideoStereo` | no | no | **yes** |
| `ScriptedCameraFlat` | no | no | **yes** |
| `scripted-cinema:` log | no | no | **yes** |
| `mainMenuStereoHold` | no | no | no *(added only in later working tree)* |

## Files / mechanisms that break menu (9AFB layer)

### INI (prep / `162657` backup — not in accepted profile)

- `MainMenuVideoStereo=1`
- `ScriptedCameraFlat=1`

Accepted INI has **neither** key.

### `d3d9.cpp` (scripted-flat, still in working tree)

- `g_mainMenuVideoStereo`, `g_scriptedCameraFlat`, INI read + log line
- `UpdateSceneModeFrame`: `ScriptedCameraFlatPresentation`, `presentationCine = engineCine || scriptedFlat`, `bootVideoStereo`, `mainMenuStereoHold` signals (later edits)
- Downstream: `FrontendMenuPresentation` uses `presentationCine` → **`MenuFlat` reason=16** with stale `cine=1` on class-2 focus (seen in log at frame ~4536)

### `vr_scene_mode.h` (later than 9AFB binary, still uncommitted)

- `ScriptedCameraFlatPresentation`, `BootMenuVideoStereo`, `mainMenuStereoHold`, `bootVideoStereo` ordering in `Controller::Advance`
- `FrontendMenuPresentation`: class **4 removed** from frontend MenuFlat (locker fix — keep for gameplay, unrelated to attract break)

### Not the menu break (keep for target candidate)

- **`da690c7`**: `RefreshNativeCinematicState`, camera-owner / restraint / gore / weapon-steady / cinematic bridge
- **`9C82`**: sprint — `camera_stabilization.h` clamp+smooth, `StabilizeMatrixBasis` outside melee-only, INI `VrTranslationStabilization=1` (trade-off: weapon bob vs sprint)
- **Locker / station** (class 4): separate uncommitted block; do not conflate with attract menu

## Target integration (what user asked for)

**Base DLL:** build at **`da690c7`** (`594C1A47…`) — rifle/Kasimov/gore/Kathryn infra **without** scripted-flat menu code.

**Add back on a branch (not in one INI dump):**

1. Sprint patch from `9C82` (rotation + optional translation — user accepted `VrTranslationStabilization=0` for walking)
2. Locker / wheel / station INI keys from later work
3. **Do not ship:** `MainMenuVideoStereo`, `ScriptedCameraFlat`, or `UpdateSceneModeFrame` scripted-flat / attract scene overrides until menu path is redesigned without `MenuFlat` + stale cine

**INI:** start from **`55A3ED4D`** (accepted) + `prepare_combined_followups.ps1` targets only (wheel, gore, rifle, `CutsceneNativeHands`, etc.) — **no** scripted-flat keys.

## Verification checklist (next PSVR2)

1. Cold start attract — full menu (must match EF3A)
2. Kathryn + bound hands (da690c7+)
3. Sprint / R1 / Kasimov (with chosen `VrTranslationStabilization`)
4. Locker + wheel (optional later layer)
