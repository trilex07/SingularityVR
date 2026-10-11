# SingularityVR menu / intro rendering root-cause audit

**Branch:** `research/menu-intro-rootcause`  
**Base:** `origin/main` (`f74210e feat(vr): first playable version`)  
**Date:** 2026-10-11  
**Mandate:** Research only. No runtime C++ changes, no Steam touches, no implementation.

---

## 1. Executive summary

Three reported symptoms point to **three separate mechanisms** in the code. This audit documents the exact code paths, the evidence behind each claim, and what remains unverified because no headset/runtime measurement has been collected on this branch.

| Symptom | Primary code path | Evidence status |
|---|---|---|
| **Cropped / oversized intro (Bink)** | `IsVideoQuad` / `DrawVideoQuadPerEye` in `spikes/view_matrix/d3d9.cpp` | Code-level confirmed: classifier is heuristic, rejects non-L8 textures, and the dimension test is dead code. Historical misfires are logged in `HISTORY.md`. Runtime miss rate is unmeasured. |
| **Duplicated / flattened menus** | `HudStereoPair` eye-remap + captured-menu provider + `VrXrSubmitMenuFlat` flattening | Code-level confirmed: `VrXrSubmitMenuFlat` returns `true` for any focused menu / class 1–4, forcing `menuFlat` submission. `HudStereoPair` already treats classes 2/3 as diegetic, so renderer and submission disagree. Actual visible duplication depends on whether the provider is active and whether suppression misses. |
| **Incorrect VR menu positioning** | `uipointer::HeadPlane` / `UpdateVrUiPointer` / `HudStereoPair` `ndcShift` clamp | Geometry suggests the clamp is generous for typical IPD/distance but could bind at close distances. No measured misalignment values exist, so this remains a hypothesis, not a confirmed defect. |

The most promising first experiment is to **stop forcing `menuFlat=true` for diegetic menus (classes 2–4)**. It is a one-line policy change, but it is **not a proven fix**: it must be evaluated against the captured-menu provider path and class-transition behavior.

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

## 3. Code-level findings

### 3.1 The Bink / intro classifier is heuristic and its dimension test is dead code

**Location:** `spikes/view_matrix/d3d9.cpp` lines 17599–17622 (`IsVideoQuad`), 17711–17745 (`DrawVideoQuadPerEye`), 17808–17941 (`Hook_DrawPrimUP`).

`IsVideoQuad` requires, simultaneously:

1. `primCount == 2` (triangle-strip quad).
2. `g_vidFrameDraws == 1` at the moment the draw is classified (i.e., this draw was the first one seen this frame by the draw hooks, not that the whole frame contains only one draw).
3. Bound texture format `D3DFMT_L8` (`kFmtL8 == 50`).

The function first returns `false` for any texture whose format is not L8:

```cpp
if (texFmt != kFmtL8) return false;
```

Because of that early return, the final expression:

```cpp
return texW && texH && rtW && rtH &&
       (texFmt == kFmtL8 || texW != rtW || texH != rtH);
```

always evaluates `texFmt == kFmtL8` as `true`. The `texW != rtW || texH != rtH` dimension check is therefore **dead code**. An L8 quad is classified regardless of whether its texture matches the render-target size.

`g_vidFrameDraws == 1` is a **draw-order check**, not a frame-total check. `VidProbeFrameEnd` (lines 12970–12997) computes the actual per-frame draw total and uses a 60-draw threshold for its own “looks like video” dump, but `IsVideoQuad` only knows whether this particular draw arrived before any other draw that reached the hooked paths.

**Reported symptom chain:**

- **Oversized / zoomed:** when classification misses, the fullscreen quad is passed through untouched. The SBS backbuffer then contains one copy of the video spanning both eye halves; OpenXR submits each eye a different half, stretched to fill the whole eye. This matches the run-160 symptom (HISTORY lines 1747–1772).
- **Cropped / doubled:** when classification fires on a non-Bink fullscreen quad, `DrawVideoQuadPerEye` scales `x` and `y` by 0.5 and recenters it. HISTORY run 167 logged 51 such misfires per second during gameplay (HISTORY lines 1761–1766).

**Evidence from the source:**

- The per-second misfire counter `g_videoQuadDraws` exists because prior tuning attempts misfired (line 17559).
- HISTORY documents four attempted conditions; the fourth was reverted because it fixed the menu and broke real video (HISTORY lines 1637–1645).
- `DrawVideoQuadPerEye` returns `false` and falls through if the vertex layout is not one it can rewrite (lines 17720–17722).
- The source comment at line 17595 states the real fix is to "hook Bink's own presentation, or identify the video by its render target."

### 3.2 The OpenXR "menu flat" bridge is applied to all focused menus / classes 1–4

**Location:** `spikes/view_matrix/d3d9.cpp` lines 5488–5494 (`VrXrSubmitMenuFlat`), 4913–4923 (`StampCaptureState`), 10760–10943 (OpenXR submission block), `spikes/view_matrix/vr_scene_mode.h`.

`VrXrSubmitMenuFlat()` currently returns `true` when:

- `g_vrUiMenu == 1` (a recognized GFx menu movie is focused), **or**
- `g_vrMenuClass` is 1–4, regardless of `g_vrUiMenu`.

When `true`, `StampCaptureState` forces:

```cpp
cap.stereo = false;
cap.dup = true;
cap.menuFlat = true;
```

The OpenXR submission then sets:

```cpp
const bool cropEyes = frame && frame->dup && !frame->stereo && frame->menuFlat && g_srcW >= 2;
```

and copies **one half** of the SBS backbuffer to **both** eye swapchains (lines 10850–10893).

**Per-eye submission details:**

- `cropEyes == true`: two separate swapchains of width `g_srcW / 2` are acquired; both eyes receive a `CopySubresourceRegion` from the **same** source half (left by default, right if `g_swapEyes` is set). Each eye's `imageRect` covers the full swapchain extent.
- `cropEyes == false && dup == true`: both eyes reference the **same** swapchain; each eye's `imageRect` is a half-width rectangle at offset 0 or `g_scWidth / 2`.

If the two halves of the SBS backbuffer are identical, the two modes are visually equivalent. If they differ, `cropEyes` discards one half.

**Why this is problematic for classes 2–4:**

- `g_vrMenuClass == 1` (`RvUIGFxMovie_IIS`) is the **boot / legal title screen**, drawn directly to the backbuffer. `HudStereoPair` splits it into left/right halves, so copying one half to both eyes gives a flat, identical image.
- `g_vrMenuClass == 2` (`RvUIGFxMovie_Main`) and `g_vrMenuClass == 3` (`RvUIGFxMovie_Pause`) are **diegetic** — GFx renders the menu into an offscreen texture mapped onto a monitor inside a 3D scene (HISTORY lines 2027–2031; `HudStereoPair` lines 17454–17463 special-case these classes to draw once into the offscreen target). The *scene* is then stereo-rendered by `StereoPair`. Forcing `menuFlat` discards the right-eye scene and copies the left eye to both eyes.
- `g_vrMenuClass == 4` (use-station, weapon-locker, etc.) keeps the world visible behind a window (`g_vrUiMenu == 1` but the scene is still rendered). Flattening the whole frame is equally inappropriate.

**Internal contradiction:** `vr_scene_mode.h` classifies `MenuFlat`, but `StereoPairBody` only gates on `g_sceneMode.Flat()` (which excludes `MenuFlat`), so the *renderer* keeps menus stereo while the *submission* flattens them.

### 3.3 Captured-menu provider behavior across class transitions

**Location:** `spikes/view_matrix/vr_menu_overlay.h` lines 285–449, `spikes/view_matrix/native_ui_input.h` lines 36–64, `spikes/view_matrix/menu_capture_owner.h`.

- `g_vrMenuStateToken` is rebuilt every time the focused movie changes: `((LONG64)++generation << 32) | kind` (`native_ui_input.h` line 50).
- `SyncVrMenuCaptureState` invalidates the captured texture and source list whenever the token changes (`vr_menu_overlay.h` lines 285–300).
- `DrawVrMenuPointer` only provides the captured menu when `g_vrCaptureOwner.CanProvide(VrMenuStateToken())` returns true, i.e., the captured surface was produced during the *current* menu generation (`menu_capture_owner.h` lines 10–12).

**Consequence:** every class transition creates a brief window in which the provider has no valid capture, so the native GFx menu renders un-suppressed. If `menuFlat` is true during that window, the native diegetic menu is flattened. If the provider later captures and starts drawing, the visual presentation switches from native-rendered to provider-drawn. That transition itself can appear as a pop or duplication.

`SuppressCapturedVrMenuDraw` (`d3d9.cpp` lines 19013–19034) only suppresses draws that bind the captured source texture on stages 0–3. If GFx samples the menu through a shader-resource path that does not use a direct stage bind, suppression misses and a native copy remains visible behind the provider quad.

### 3.4 `ndcShift` clamp — geometry, not measured defect

**Location:** `spikes/view_matrix/ui_pointer.h` lines 16–21 (`HeadPlane`), `spikes/view_matrix/vr_menu_overlay.h` lines 267–275 (`UpdateVrUiPointer`), `spikes/view_matrix/d3d9.cpp` lines 17490–17501 (`HudStereoPair`).

`HeadPlane` builds one plane centred at the midpoint of the two eyes, using the **symmetric** tangents `g_targetTanX` / `g_targetTanY` derived from the headset FOV (`d3d9.cpp` lines 19349–19351). `VrUiProjectEye` projects that centre through each eye's actual pose.

For a head-relative plane at distance `D` and half-IPD `d`, the angular offset of the plane centre as seen from one eye is approximately `arctan(d / D)`.

Example, IPD = 0.063 m → `d = 0.0315 m`, `D = 1.2 m`:

```
θ ≈ atan(0.0315 / 1.2) ≈ 0.0262 rad ≈ 1.5°
```

The symmetric horizontal field is `2 * atan(g_targetTanX)`. For a 90° total horizontal FOV, `atan(g_targetTanX) ≈ 0.785 rad`, so the half-field width is `0.785 rad`. The screen-U offset is:

```
ΔU ≈ θ / (2 * atan(g_targetTanX)) ≈ 0.0262 / 1.571 ≈ 0.0167
```

`ndcShift = 2 * (U - 0.5)`, so for this example `ndcShift ≈ 0.033`, which is **inside** the ±0.04 clamp. At `D = 0.8 m` with the same IPD, `ndcShift ≈ 0.056`, which would hit the clamp. Closer distances or larger IPDs could bind.

**Status:** the clamp is a bounded correction, not an unbounded error. Whether it causes visible misalignment depends on the user's configured `VrMenuDistanceMm`, actual IPD, and headset FOV. No measured misalignment values exist in `HISTORY.md` or the source, so this is a **hypothesis to test**, not a confirmed defect.

### 3.5 `VideoStereo=0` does not disable the rewrite while VR mode is active

**Location:** `spikes/view_matrix/d3d9.cpp` lines 17914–17922 (`Hook_DrawPrimUP`).

The video-rewrite gate is:

```cpp
if (InterlockedCompareExchange(&g_sceneFlatEnabled, 0, 0) && videoQuad &&
    (VrModeOn() || InterlockedCompareExchange(&g_videoStereo, 0, 0))) {
```

`VrModeOn()` is true whenever `g_dupDraws != 0`. Because the condition is `VrModeOn() || g_videoStereo`, `VideoStereo=0` only prevents the rewrite when **VR mode is off**. Once the user has toggled VR on, the Bink rewrite runs regardless of the `VideoStereo` INI value.

This contradicts the documented intent in `HISTORY.md` (lines 1761–1772) that `VideoStereo=0` "turns the whole feature off and restores the original behaviour." The most likely explanation is that the rewrite was added before `StartInVr=1` became the default: early splash screens run before `g_dupDraws` is set, so `g_videoStereo` was the only way to enable the rewrite for the intro. After VR mode activates, `VrModeOn()` takes over.

---

## 4. Hypotheses (not yet proven)

### 4.1 Captured-menu provider double-draw

If `SuppressCapturedVrMenuDraw` misses native menu draws (because the texture is sampled indirectly or on a stage > 3), the native menu remains visible behind/in front of the provider quad. This would read as duplication or ghosting.

**Falsification:** enable `g_menuDrawTrace=1` and look for menu-source draws (flags bit 4) that also carry path bit 2 (`HudStereoPair`) while the provider is active (path bit 4).

### 4.2 `g_vrMenuPanel.canvas` UV crop is never initialized from the captured texture

`VrMenuPanel.canvas` defaults to full UV `[0,1]`. `CaptureVrMenuTarget` selects the texture whose aspect is closest to 16:9 but never sets the crop. If the captured target is not exactly 16:9, or GFx renders into a sub-rectangle, the provider quad stretches or shows empty borders.

**Falsification:** dump `g_vrMenuTexture` width/height and the rendered quad UVs, then compare to the visible menu extents.

### 4.3 `NeedsMonoPair()` and the state-machine comment disagree

`vr_scene_mode.h` line 30 says "menu never requests mono rendering because its renderer owns its own flat presentation," but `NeedsMonoPair()` returns `true` for `Mode::MenuFlat` (line 45). The function is only used in the unit test, so it is dead code, but it documents an unresolved design question.

### 4.4 Class-1-only `menuFlat` may regress if class 1 is not perfectly duplicated

For class 1, `HudStereoPair` applies `menuInsetPct` scaling and per-eye `ndcShift`. If the boot/title menu is not perfectly centred, the left and right halves could differ. Removing `menuFlat` for class 1 would then present different content to each eye.

---

## 5. Architectural comparison

### 5.1 Dishonored-VR

From `docs/REFERENCE_VR_IMPLEMENTATIONS.md` (P0-2):

- Captures UI to **private sink targets** per element.
- Submits each captured element as an `XrCompositionLayerQuad` at a tunable world distance.
- Uses a 4-term classification rule (rt0 == backbuffer, full viewport, depth off, blending on) plus a vertex-shader region probe.

**Lesson:** Singularity's provider is conceptually similar but lacks Dishonored's "private target + copy-after-clear + previous-slot delivery" discipline. Moving the provider to a real OpenXR quad layer would remove the `HudStereoPair` / `menuFlat` interaction.

### 5.2 KHARVOX

KHARVOX handles intro/logo content through a **standalone `KharvoxIntro.exe` launcher**. The in-game Bink path is not referenced. A robust intro solution may need to run **before** the game renders its first frame, rather than classifying the game's Bink draws.

### 5.3 vrframework

From `references/vrframework/guides/11-hud-ui-and-menus-in-vr.md`:

- **Anvil:** hooks the engine UI-viewport calc and rescales the HUD box toward the centre.
- **Creation Engine / Starfield:** classifies Scaleform movies by file URL hash, maintains a per-frame HUD-vs-menu tally, and forces `perspective = 0` for real menus.
- **REFramework:** renders its own UI to a texture and submits it as a dashboard overlay.

**Lesson:** Singularity already classifies movies by class name (`native_ui_input.h` `MenuClass`), but it does **not** use that classification consistently in submission.

---

## 6. Proposed experiment (not a proven fix)

### 6.1 Candidate change: restrict `menuFlat` to class-1 backbuffer menus

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
    // and should keep the stereo scene intact.
    const LONG menuClass = InterlockedCompareExchange(&g_vrMenuClass, 0, 0);
    return menuClass == 1;
}
```

**Expected effects:**

- **Classes 2–4 without provider active:** the diegetic scene is submitted in stereo (each eye gets its own half). This is the desired outcome for the reported duplication/flattening symptom.
- **Classes 2–4 with provider active:** the provider draws the same flat quad into both halves of the SBS buffer. With `menuFlat=false`, the submission uses one shared swapchain with half-rects per eye. Because both halves are identical, the visual result is the same; only the copy path differs.
- **Class 1:** remains `menuFlat=true`, so the boot/title screen continues to be copied from one half to both eyes.

**Possible regressions:**

- If class 1 content is not perfectly duplicated between halves (e.g., due to per-eye `ndcShift` or off-centre UI), removing `menuFlat` for class 1 would show inconsistent content per eye. The change above keeps class 1 flat, so this risk is limited to whether class 1 should also be re-evaluated later.
- During class transitions, the provider has no valid capture for a few frames. With `menuFlat=false` for classes 2–4, those transient native frames are submitted in stereo, which is correct for diegetic menus but may look different from the provider-drawn frames that follow.
- If any class-2/3 menu is meant to be displayed as a flat overlay by design, this change would incorrectly leave it in stereo. The audit assumes classes 2/3 are diegetic based on `HudStereoPair`'s special case; if that assumption is wrong, the change must be narrowed.

### 6.2 Secondary experiment: expose `ndcShift` clamp for measurement

Make the clamp an INI value defaulting to 0.04. Test at the user's actual `VrMenuDistanceMm` and IPD to determine whether the clamp binds.

### 6.3 Research direction: Bink classifier diagnostics

Before rewriting the classifier, add a bounded counter for draws that look like Bink (2-prim, L8, fullscreen quad) but fail the `g_vidFrameDraws == 1` condition. This will tell us whether the dominant miss mode is draw-order, format, or vertex layout.

---

## 7. Verification plan

Testing can use the Steam XR simulator (`docs/STEAM_XRSIM_RUNNER_PLAYBOOK.md`) where convenient; real Steam acceptance is owned by the main integration branch and is not a mandatory gate for this research audit.

### 7.1 Gate 1: build and unit-test

1. Build the x86 `/O2` DLL from the branch.
2. Run `spikes/view_matrix/tests/test-vr-scene-mode.cpp`.
3. Verify PE32 / SHA provenance if the change is later picked up by MAIN.

### 7.2 Gate 2: simulator / runtime — menu class logging

1. Launch with `SceneModeTrace=1`.
2. Progress through:
   - boot / IIS title screen (class 1),
   - main menu (class 2),
   - pause menu (class 3),
   - a use-station / weapon-locker (class 4).
3. Capture:
   - `VR UI focus: ... class=... menu=...`
   - `SceneMode: ... -> ... menu=...`
   - `menu: XR bridge submits one identical source crop to both eyes ...`

**Acceptance for the experiment:** the bridge log appears for class 1; it must **not** appear for classes 2–4 (`cropEyes=false`).

### 7.3 Gate 3: simulator / runtime — intro video

1. Run the intro sequence.
2. Watch for the `video: Bink quad detected ...` log and the per-second `video quads drawn per eye` counter.
3. Compare frames where the intro is correct vs. oversized/cropped.

**Acceptance:** the per-second counter remains non-zero during real Bink playback and zero during gameplay. A new diagnostic should identify whether misses are due to `g_vidFrameDraws != 1`.

### 7.4 Gate 4: runtime — duplicated menu check

1. Open the main menu and pause menu.
2. Enable `g_menuDrawTrace=1` and dump.
3. Inspect `MDG` entries for menu-source draws that reach `HudStereoPair` while the provider is active.

**Acceptance:** menu-source draws are either suppressed or drawn by the provider, not both.

### 7.5 Gate 5: physical headset — comfort and alignment (optional, MAIN-owned)

1. With the candidate change, verify the main/pause menu monitor has stereo depth.
2. Test the `ndcShift` clamp at the user's configured distance to confirm or reject the positioning hypothesis.

---

## 8. Falsification tests

| Hypothesis | What would prove it wrong |
|---|---|
| Restricting `menuFlat` to class 1 improves diegetic menus. | Apply only the policy change; if classes 2–3 still look duplicated/cropped, the defect is elsewhere (e.g., provider suppression or capture failure). |
| The `ndcShift` clamp causes menu misalignment. | Expose the clamp and set it to a large value; if alignment does not improve, the plane math or projection is wrong, not the clamp. |
| Bink classifier misses are due to `g_vidFrameDraws != 1`. | Add the diagnostic counter; if it stays zero during missed frames, the cause is format or vertex layout, not draw order. |
| The captured-menu provider is responsible for duplication. | Disable the provider (`g_vrMenuOverlay=0`); if duplication disappears, the provider is the source. |

---

## 9. Recommended next steps

1. **Run the class-1-only `menuFlat` experiment** under simulator/headset and measure the regression items listed in §6.1.
2. **Add the Bink miss diagnostic** before attempting any classifier rewrite.
3. **Resolve `vr_scene_mode.h` inconsistency:** decide whether `MenuFlat` implies mono submission and update the comment, `NeedsMonoPair()`, and the unit test.
4. **Evaluate Dishonored-style quad-layer submission** as the longer-term architectural fix.

---

## 10. Notes on failed / avoided approaches

Per `HISTORY.md` and source comments, the following are **not recommended**:

- Adding a fifth geometric condition to `IsVideoQuad` (line 17595).
- Guessing new `VrMenuDistanceMm` or `MenuInsetPct` values to fix positioning without measured evidence.
- Broadening `HudStereoPair` to remap world-space/diegetic UI (run 151 already proved this creates the doubled menu).
