# SingularityVR menu / intro rendering root-cause audit

**Branch:** `research/menu-intro-rootcause`  
**Worktree:** `C:\Users\Trilex\vr-mods\SingularityVR-menu-intro-rootcause`  
**Base:** `origin/main` (`f74210e feat(vr): first playable version`)  
**Date:** 2026-10-11  
**Mandate:** Research only. No runtime C++ changes, no Steam touches, no implementation.

---

## 1. Executive summary

Three reported symptoms are caused by **two independent decision points** that treat menus/video inconsistently, plus one fragile classifier that has been explicitly marked "still wrong in both directions" by the project itself:

| Symptom | Primary code path | Status |
|---|---|---|
| **Cropped / oversized intro (Bink)** | `IsVideoQuad` / `DrawVideoQuadPerEye` in `spikes/view_matrix/d3d9.cpp` | **Proven defect** — heuristic classifier misses real Bink frames and the rewrite assumes a specific NDC quad. |
| **Duplicated menus** | `HudStereoPair` eye-remap + captured-menu provider + `VrXrSubmitMenuFlat` flattening | **Proven defect** — the XR bridge is applied to menu classes that are *not* flat backbuffer menus, and the capture/suppress path can leave a second native copy visible. |
| **Incorrect VR menu positioning** | `uipointer::HeadPlane` / `UpdateVrUiPointer` / `HudStereoPair` `ndcShift` clamp | **Confirmed design flaw** — symmetric-tangent plane + per-eye pose mismatch is papered over by a ±0.04 NDC clamp that becomes the error when the true offset is larger. |

The smallest fix that removes the most visible failure is to **stop forcing `menuFlat=true` for diegetic menus (classes 2–4)** and instead let them submit as normal stereo. This is a one-line policy change in `VrXrSubmitMenuFlat()`. It does not require new scale/distance values.

---

## 2. Scope and method

### Files read

- `spikes/view_matrix/d3d9.cpp` — main D3D9 hook, stereo duplication, Bink handling, OpenXR submission.
- `spikes/view_matrix/vr_menu_overlay.h` — captured-GFx menu provider, laser, VR settings row.
- `spikes/view_matrix/vr_scene_mode.h` — flat/cutscene/video/menu state machine.
- `spikes/view_matrix/ui_pointer.h` — head-relative plane math, ray-plane intersection.
- `spikes/view_matrix/native_ui_input.h` — native GFx movie classification / cursor injection.
- `spikes/view_matrix/menu_capture_owner.h` — menu-capture token validity.
- `spikes/view_matrix/menu_draw_trace.h` — per-draw diagnostic census.
- `spikes/view_matrix/tests/test-vr-scene-mode.cpp` — state-machine unit tests.
- `HISTORY.md` — prior failed attempts (runs 144–151, 159–160, 164–169, 176–177, 211).
- `docs/CUTSCENE_FLAT_MODE.md`, `docs/REFERENCE_VR_IMPLEMENTATIONS.md`.
- `references/vrframework/guides/10-submission-and-the-taa-problem.md` and `11-hud-ui-and-menus-in-vr.md`.
- `references/KHARVOX/tools/test_vr_intro_host.ps1` (intro launcher ABI only; no in-game Bink signal).

### What was not done

- No C++ source edits.
- No Steam, runtime, or headset interaction.
- No new heuristic thresholds or magic scale/distance values proposed.

---

## 3. Confirmed defects

### 3.1 The Bink / intro classifier is heuristic and documented to misfire

**Location:** `spikes/view_matrix/d3d9.cpp` lines 17599–17622 (`IsVideoQuad`), 17711–17745 (`DrawVideoQuadPerEye`), 17808–17941 (`Hook_DrawPrimUP`).

`IsVideoQuad` requires, simultaneously:

1. `primCount == 2` (triangle-strip quad).
2. `g_vidFrameDraws == 1` (this quad is the **first and only** draw of the frame).
3. Bound texture format `D3DFMT_L8` (`kFmtL8 == 50`), **or** texture size ≠ render-target size.

The comment at lines 17587–17598 explicitly states:

> "Three conditions were bolted on here, each after a misfire, and it is still wrong in both directions… Do not add a condition to this function. It needs a real signal: hook Bink's own presentation, or identify the video by its render target."

**Why this causes the reported intro problems:**

- **Oversized / zoomed:** when the classifier misses a Bink frame, the fullscreen quad is passed through untouched. The SBS backbuffer then contains one copy of the video spanning both eye halves; OpenXR submits each eye a different half, stretched to fill the whole eye. This was the original run-160 symptom (HISTORY lines 1747–1772).
- **Cropped / doubled:** when the classifier fires on a non-Bink fullscreen quad (e.g., a bloom/DOF pass), `DrawVideoQuadPerEye` scales `x` and `y` by 0.5 and recenters it. A pass that was already authored for the full target is now halved in both axes, producing black bars or cropped content. HISTORY run 167 logged 51 misfires per second during gameplay (HISTORY lines 1761–1766).
- **First-draw fragility:** `g_vidFrameDraws == 1` is violated by any earlier draw in the frame — loading spinners, splash overlays, subtitles, or a single UI element. The classifier then silently fails and the video spans the seam.

**Evidence this is proven, not hypothetical:**

- The per-second misfire counter `g_videoQuadDraws` exists specifically because every prior tuning attempt misfired (line 17559).
- HISTORY documents four attempted conditions; the fourth was reverted because it fixed the menu and broke real video (HISTORY lines 1637–1645).
- `DrawVideoQuadPerEye` returns `false` and falls through if the vertex layout is not one it can rewrite (lines 17720–17722), leaving the video spanning the seam.

### 3.2 The OpenXR "menu flat" bridge is applied to diegetic menus

**Location:** `spikes/view_matrix/d3d9.cpp` lines 5488–5494 (`VrXrSubmitMenuFlat`), 4913–4923 (`StampCaptureState`), 10760–10943 (OpenXR submission block), `spikes/view_matrix/vr_scene_mode.h`.

`VrXrSubmitMenuFlat()` currently returns `true` for:

- any frame where `g_vrUiMenu == 1` (a recognized GFx menu movie is focused), **and**
- any frame where `g_vrMenuClass` is 1–4, regardless of `g_vrUiMenu`.

When it returns `true`, `StampCaptureState` forces:

```cpp
cap.stereo = false;
cap.dup = true;
cap.menuFlat = true;
```

and the OpenXR submission path copies **one half** of the SBS backbuffer to **both** eye swapchains (lines 10850–10893).

**Why this is wrong for classes 2–4:**

- `g_vrMenuClass == 1` (`RvUIGFxMovie_IIS`) is the **boot / legal title screen**, drawn directly to the backbuffer. `HudStereoPair` splits it into left/right halves, so copying one half to both eyes gives a flat, identical image. Correct.
- `g_vrMenuClass == 2` (`RvUIGFxMovie_Main`) and `g_vrMenuClass == 3` (`RvUIGFxMovie_Pause`) are **diegetic** — GFx renders the menu into an offscreen texture that is mapped onto a monitor inside a 3D scene (HISTORY lines 2027–2031; `HudStereoPair` lines 17454–17463 explicitly special-cases these classes to draw once into the offscreen target). The *scene* is then stereo-rendered by `StereoPair`. Forcing `menuFlat` discards the right-eye scene and copies the left eye to both eyes. The monitor therefore appears only as the left-eye view, flattened and misaligned for the right eye.
- `g_vrMenuClass == 4` (use-station, weapon-locker, etc.) keeps the world visible behind a window (`g_vrUiMenu == 1` but scene still rendered). Flattening the whole frame is equally wrong.

**Evidence this is a real code path:**

- `HudStereoPair` already knows classes 2/3 are different: it detects `!g_rtIsBackbuffer` and draws them once without eye-remap (lines 17454–17463).
- `vr_scene_mode.h` classifies `MenuFlat` but `StereoPairBody` only gates on `g_sceneMode.Flat()` (which excludes `MenuFlat`), so the *renderer* keeps menus stereo while the *submission* flattens them. This is an internal contradiction.
- The submission logs once: `"menu: XR bridge submits one identical source crop to both eyes"` (lines 10887–10890), confirming the bridge fires for any `menuFlat` frame.

### 3.3 The VR menu plane is built from symmetric tangents and then clamped

**Location:** `spikes/view_matrix/ui_pointer.h` lines 16–21 (`HeadPlane`), `spikes/view_matrix/vr_menu_overlay.h` lines 267–275 (`UpdateVrUiPointer`), `spikes/view_matrix/d3d9.cpp` lines 17490–17501 (`HudStereoPair` `ndcShift`).

`HeadPlane` builds the menu plane using:

```cpp
p.halfWidth  = distance * tanHalfX;
p.halfHeight = distance * tanHalfY;
```

where `tanHalfX` / `tanHalfY` are the **symmetric** OpenXR FOV tangents (`g_targetTanX` / `g_targetTanY`). The plane centre is the midpoint of the two eye positions (lines 267–268). This plane is head-locked and uses a single basis for both eyes.

`VrUiProjectEye` then projects the same plane centre through each eye's actual pose (which includes the per-eye position offset). For a real asymmetric headset FOV or a large IPD, the projected centre can deviate from `u = 0.5`.

`HudStereoPair` applies a corrective `ndcShift`:

```cpp
const float centre = g_vrUiEyeCentreU[eye] / 1000000.0f;
float ndcShift = 2.0f * (centre - .5f);
if (ndcShift >  0.04f) ndcShift =  0.04f;
if (ndcShift < -0.04f) ndcShift = -0.04f;
```

**Why this is a defect:**

- The clamp is a band-aid for "a bad plane centre used to shove the whole menu off the left of the eye" (line 17495). When the true centre offset exceeds ±0.02 screen-U (which corresponds to ±0.04 NDC), the clamp becomes the menu's position error.
- At `VrMenuDistanceMm = 1200` mm and a 70 mm IPD, the eye-offset angle is ~3.3°. At a 90° horizontal FOV, that is already ~0.037 U, close to the clamp edge. Any closer distance or wider IPD pushes it over.
- The plane is symmetric, but real OpenXR per-eye FOVs are usually asymmetric. The code uses the symmetric tangents to place the plane, then tries to compensate after the fact instead of placing the plane relative to each eye's actual frustum.

**Status:** proven design flaw. The clamp directly bounds the menu alignment error; the bound is simply too small for some configurations.

---

## 4. Hypotheses (not yet proven)

These are plausible failure modes that should be tested before being promoted to defects.

### 4.1 Captured-menu provider can double-draw with native menu

`DrawVrMenuPointer` (lines 357–448) redraws a captured GFx texture as a head-relative quad. `SuppressCapturedVrMenuDraw` (lines 19013–19034) drops native draws that sample a captured source texture. The suppression gate requires:

- `g_vrUiMenu == 1`
- `MenuSourceBoundAny()` (texture bound on stages 0–3)
- `g_vrCaptureOwner.CanProvide(VrMenuStateToken())`
- `VrModeOn()`
- not in shadow pass
- not in provider drawing
- on the scene target

If GFx samples the menu through a shader-resource path that does not bind the texture to stages 0–3, or if the captured texture is bound through a different intermediate surface, suppression misses and the native menu remains visible behind/in front of the provider quad. This would read as a duplicated/ghosted menu.

**Falsification:** enable `g_menuDrawTrace=1` and compare the set of draws that reach `HudStereoPair` (path bit 2) while the provider is active (path bit 4). A non-zero count of un-suppressed menu-source draws is the failure signature.

### 4.2 `g_vrMenuPanel.canvas` UV crop is never initialized from the captured texture

`VrMenuPanel` carries a `CanvasMap canvas` with default `cropU0=cropV0=0`, `cropU1=cropV1=1`, `flipY=false` (`ui_pointer.h` lines 42, 67–68). `CaptureVrMenuTarget` selects the texture whose aspect is closest to 16:9 but never sets the crop to match the actual GFx viewport. If the captured render target is not exactly 16:9, or if GFx renders into a sub-rectangle, the provider quad uses stretched or empty UVs.

**Falsification:** dump the selected `g_vrMenuTexture` width/height alongside the rendered quad UVs and compare to the visible menu extents.

### 4.3 `NeedsMonoPair()` and the state-machine comment disagree

`vr_scene_mode.h` line 30 says "menu never requests mono rendering because its renderer owns its own flat presentation", but `NeedsMonoPair()` returns `true` for `Mode::MenuFlat` (line 45). The function is only used in the unit test, so it is currently dead code, but it documents an unresolved design question: is `MenuFlat` supposed to flatten submission or not? The current behavior (renderer keeps stereo, submission flattens) is inconsistent with both readings.

**Falsification:** decide the intended policy and make the code, the comment, and the test agree.

---

## 5. Architectural comparison

### 5.1 Dishonored-VR (reference only, no code copied)

From `docs/REFERENCE_VR_IMPLEMENTATIONS.md` (P0-2):

- Captures UI to **private sink targets** per element, not to the backbuffer.
- Submits each captured element as an `XrCompositionLayerQuad` at a tunable world distance.
- Uses a 4-term classification (rt0 == backbuffer, full viewport, depth off, blending on) plus a vertex-shader region probe.
- The menu is therefore **one authoritative quad** at a comfortable distance, identical in both eyes, without touching engine stereo.

**Lesson for Singularity:** the current capture/provider path is conceptually similar, but it lacks Dishonored's "private target + copy-after-clear + previous-slot delivery" discipline and instead tries to suppress native draws on the scene target. Moving the provider fully to an OpenXR quad layer would remove the `HudStereoPair` / `menuFlat` interaction entirely.

### 5.2 KHARVOX

KHARVOX handles intro/logo content through a **standalone `KharvoxIntro.exe` launcher** (`references/KHARVOX/tools/test_vr_intro_host.ps1`). The in-game Bink path is not part of the referenced KHARVOX integration. This tells us that a robust intro solution may need to run **before** the game renders its first frame, rather than classifying the game's Bink draws heuristically.

### 5.3 vrframework

From `references/vrframework/guides/11-hud-ui-and-menus-in-vr.md`:

- **Anvil:** hooks the engine's UI-viewport calc and rescales the HUD box toward the center, keeping width/height independent so aspect ratio is preserved.
- **Creation Engine / Starfield:** classifies Scaleform movies by file URL hash, maintains a per-frame HUD-vs-menu tally double-buffered by `modulino`, and forces `perspective = 0` for real menus so they stay flat.
- **REFramework:** renders its own UI to a texture and submits it as a SteamVR/OpenVR dashboard overlay (world-space quad) with ray-intersection input.

**Lesson for Singularity:** Singularity already classifies movies by class name (`native_ui_input.h` `MenuClass`), but it does **not** use that classification consistently in submission. Starfield's rule — "menus are flat panels, no per-eye parallax" — is partially implemented (the `menuFlat` bridge), but it is applied to the wrong menu classes.

---

## 6. Proposed minimal fix

### 6.1 Primary fix: restrict `menuFlat` to class-1 backbuffer menus

**File:** `spikes/view_matrix/d3d9.cpp`  
**Function:** `VrXrSubmitMenuFlat()` (lines 5488–5494)

Change:

```cpp
static bool VrXrSubmitMenuFlat() {
    if (Rd(g_vrUiMenu)) return true;
    const LONG menuClass = InterlockedCompareExchange(&g_vrMenuClass, 0, 0);
    if (menuClass >= 1 && menuClass <= 4) return true;
    return false;
}
```

To:

```cpp
static bool VrXrSubmitMenuFlat() {
    // Only the IIS boot/title menu (class 1) is a flat backbuffer overlay.
    // Main (2), pause (3) and station/dialog (4) menus are diegetic or windowed
    // and must keep the stereo scene intact.
    const LONG menuClass = InterlockedCompareExchange(&g_vrMenuClass, 0, 0);
    return menuClass == 1;
}
```

**Why this is the smallest fix:**

- One policy change, no new constants, no new render paths.
- It makes `StampCaptureState` tell the truth: class 1 is flat, classes 2–4 stay stereo.
- It aligns with the existing `HudStereoPair` special case that already treats classes 2/3 as diegetic.
- It does not touch the captured-menu provider; if the provider is active for a class-2/3 menu, both eye halves of the SBS buffer contain the same flat quad, and normal stereo submission still shows each eye the same image (just without the unnecessary `cropEyes` copy).

### 6.2 Secondary fix: remove or enlarge the `ndcShift` clamp

**File:** `spikes/view_matrix/d3d9.cpp`  
**Function:** `HudStereoPair` (lines 17490–17501)

Make the clamp configurable or replace it with a per-eye plane centre. The immediate, smallest change is to expose the clamp as an INI value (defaulting to the current 0.04) so it can be falsified:

```cpp
const float maxShift = Rd(g_vrMenuNdcShiftMax); // e.g. 0.04f default
if (ndcShift >  maxShift) ndcShift =  maxShift;
if (ndcShift < -maxShift) ndcShift = -maxShift;
```

A better architectural fix (larger) is to build the plane per-eye in `UpdateVrUiPointer` rather than using a single head-relative plane and a post-hoc shift. The audit recommends the configurable clamp as the first falsification step.

### 6.3 Tertiary / research direction: replace Bink heuristic with render-target identity

The project already knows the correct fix: "hook Bink's own presentation, or identify the video by its render target" (line 17595). This is **not** the smallest fix and is listed as a follow-up. The immediate action is to add a bounded diagnostic that logs missed Bink frames:

- In `IsVideoQuad`, when `primCount == 2`, texture format is L8, but `g_vidFrameDraws != 1`, increment a new counter and print it once per second. This will quantify how often the first-draw condition is the failure.

---

## 7. Verification plan

All tests can be done in the Steam XR simulator / `tools/singularityvr-test-bridge.ps1` per `AGENTS.md`. No headset is required for the automated gates; a short physical headset pass is the final acceptance gate.

### 7.1 Gate 1: build and unit-test

1. Build the x86 `/O2` DLL from the branch.
2. Run `spikes/view_matrix/tests/test-vr-scene-mode.cpp` (already passes; update the test if the policy change makes `NeedsMonoPair()` semantics explicit).
3. Verify PE32 / SHA provenance.

### 7.2 Gate 2: simulator — menu class logging

1. Launch via `tools/steam-xrsim-singularity.ps1` with `SceneModeTrace=1`.
2. Progress through:
   - boot / IIS title screen (class 1),
   - main menu (class 2),
   - pause menu (class 3),
   - a use-station / weapon-locker (class 4).
3. For each state, capture the log lines:
   - `VR UI focus: ... class=... menu=...`
   - `SceneMode: ... -> ... menu=...`
   - `menu: XR bridge submits one identical source crop to both eyes ...`

**Acceptance:** the bridge log must appear **only** for class 1. Classes 2–4 must submit as normal stereo (`cropEyes=false`).

### 7.3 Gate 3: simulator — intro video

1. Run the intro sequence with `VideoStereo=1`.
2. Watch for the `video: Bink quad detected ...` log and the per-second `video quads drawn per eye` counter.
3. Compare frames where the intro is correct vs. oversized/cropped.

**Acceptance:** the per-second counter must remain non-zero for the duration of real Bink playback and must read zero during gameplay. Any frame where it reads zero during Bink is a classifier miss to investigate with the new diagnostic counter.

### 7.4 Gate 4: simulator — duplicated menu check

1. Open the main menu and pause menu.
2. Enable `g_menuDrawTrace=1` and dump.
3. Inspect `MDG` entries for menu-source draws (flags bit 4 set) that also have path bit 2 (`HudStereoPair`) while the provider is active (path bit 4).

**Acceptance:** menu-source draws must be either suppressed or drawn by the provider, not both.

### 7.5 Gate 5: physical headset — comfort and alignment

1. With the primary fix applied, verify the main/pause menu monitor has stereo depth and does not look like a flat photograph copied to both eyes.
2. With the `ndcShift` clamp enlarged (e.g., 0.10), verify the VR menu quad centers under the headset crosshair at the configured distance.
3. Revert the clamp to 0.04 and confirm the original misalignment returns (falsification).

---

## 8. Falsification tests

| Hypothesis | What would prove it wrong |
|---|---|
| Restricting `menuFlat` to class 1 fixes duplicated/cropped diegetic menus. | Apply only the policy change; if classes 2–3 still look duplicated/cropped, the defect is elsewhere (e.g., provider suppression). |
| The `ndcShift` clamp causes menu misalignment. | Set clamp to a large value (e.g., 0.20); if alignment does not improve, the plane math is wrong, not the clamp. |
| The Bink classifier misses are due to `g_vidFrameDraws != 1`. | Add the diagnostic counter; if it stays zero during missed frames, the cause is texture format or vertex layout, not draw count. |
| The captured-menu provider is responsible for duplication. | Disable the provider (`g_vrMenuOverlay=0`); if duplication disappears, the provider is the source. |

---

## 9. Recommended next steps

1. **Implement the class-1 `menuFlat` restriction** as the first, smallest change. It is measurable, reversible, and matches the existing `HudStereoPair` special case.
2. **Add the Bink miss diagnostic** before attempting any classifier rewrite.
3. **Make `vr_scene_mode.h` consistent:** decide whether `MenuFlat` implies mono submission and update the comment, `NeedsMonoPair()`, and the unit test accordingly.
4. **Evaluate Dishonored-style quad-layer submission** as the longer-term architectural fix for both menu duplication and positioning.

---

## 10. Notes on failed / avoided approaches

Per HISTORY.md and the source comments, the following approaches are **explicitly not recommended**:

- Adding a fifth geometric condition to `IsVideoQuad` (line 17595).
- Guessing new `VrMenuDistanceMm` or `MenuInsetPct` values to fix positioning (the defect is in the plane-to-clip mapping, not the distance).
- Broadening `HudStereoPair` to remaps world-space/diegetic UI (run 151 already proved this creates the doubled menu).
