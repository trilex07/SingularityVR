# Pause menu head overlay (class 3)

## Problem

Physically verified stacks (PR #5/#6 + PR #7) left the **native pause GFx** on the in-world monitor: readable only after awkward head movement. The **VR SETTINGS** synthetic row is head-relative; the interactive pause list is not.

## Evidence (2026-10-11 physical logs)

| Menu | `VR menu sink` (private GFx RT) | `MenuDrawTrace` during focus |
|------|----------------------------------|------------------------------|
| Main (`class=2`) | `1280x720` registered | private `1280x720` + scene sampling |
| Pause (`class=3`) | **none** | **only** scene-sized colour targets (`5264x2684` class) |

Main menu path: GFx composes into a **private** colour target → `CaptureVrMenuTarget` on `SetRenderTarget` → head-relative provider quad + suppress diegetic monitor sampling.

Pause path: orthographic GFx batches reach `HudStereoPair` on the **scene-sized** target (`menu mono: class=3`), so dimension-only scene capture is **not** the same texture identity as main and was correctly rejected on PR #6.

## Approach (this branch)

**Do not** capture the full scene colour target.

When `[Render] PauseHeadMenu=1` (default **0** until headset acceptance):

1. **Primary:** `DrawIndexedPrimUP` batches matching MDG flash structure (stride 8/12, GFx blend, prim cap) **and** a learned **PS+VS+decl** triplet per stride (learn only while redirect context is active — not during main menu). Clip gate uses GFx `c5-c8` shape plus centered pause ortho (`|c12|,|c13| ≤ 0.88`). Does **not** include large stride-32 scene batches or unlearned shaders.
2. **Fallback:** same `HudStereoPair` mono path as before for draws that miss the narrow DIPUP filter.
3. Redirect eligible draws into a **pooled `1280x720`** RT; remap `c5-c8` and scissor from scene SBS viewport to pool canvas (L1: MDG fixture regression). **`PauseHeadPoolOutcome`:** pooled success consumes the draw; bind failure returns **scene fallback** (caller runs original draw); pooled draw failure restores D3D state then falls through — never counted as `poolOk`.
4. Reuse **`CaptureVrMenuTarget` + `DrawVrMenuPointer`** provider path. Dispatch log includes `poolFallback` separate from `poolOk` / `poolFail`.
5. Set `PauseHeadMenu=0` to restore legacy diegetic-only pause instantly.

## Falsification (L3, before `PHYSICALLY_VERIFIED`)

1. With `PauseHeadMenu=1`, open pause: log must show `PauseHeadMenu: pooled GFx target` once, then `VR menu sink: private GFx candidate 1280x720`, and `pauseMenuProvided` > 0 in the VR counter dump.
2. **False pass:** only the VR SETTINGS row moves while the GFx list stays on the monitor (`pauseMenuRedirectDraws` > 0 but `pauseMenuProvided` == 0).
3. **False pass:** main menu / IIS / Bink / stations change with `PauseHeadMenu=0` (regression control).

## Out of scope

- OpenXR submission policy, IIS inset, VR SETTINGS row placement (landed via menu integration PR).
- Scene-sized pause capture (`CapturePauseDiegeticTarget` and similar).

## Attribution

Builds on captured-menu provider work from merged menu-flat policy (PR #5) and integration cherry-picks of PR #6 / PR #7.
