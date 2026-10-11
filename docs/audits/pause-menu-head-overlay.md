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

1. **Primary:** `DrawIndexedPrimUP` batches matching MDG flash structure (stride 8/12, GFx blend, prim cap). Identity (**PS+VS+decl**) is learned only after structural match, `FlashMatrixShape`, the centered clip gate (`|c12|,|c13| ≤ 0.88`), and active pause redirect context. Does **not** include large stride-32 scene batches.
2. **Fallback:** same `HudStereoPair` mono path as before for draws that miss the narrow DIPUP filter.
3. Redirect eligible draws into a **pooled `1280x720`** RT. GFx positions are int16; c5 maps them to clip; D3D9 maps clip to viewport pixels. The pool uses a **uniform contain** of the source viewport into 1280×720 (5264×2684 letterboxes vertically). Scissor uses that same pixel map. This is not an independent `(pool/scene)` scale of c5.
4. **`PauseHeadPoolResult`:** pooled success returns the draw HRESULT and consumes the draw. Bind failure with a successful restore is a scene fallback (one original draw). A failed pooled draw or a failed state restore returns that HRESULT and does **not** draw again.
5. Set `PauseHeadMenu=0` to restore legacy diegetic-only pause instantly.

## Limitation (not ready for visual acceptance)

MDG on the 2026-10-11 pause retest shows the stride-8/12 DIPUP signature as **fragments** (for example MDG 137: 42 prims at one row; MDG 263: 10 prims, different scale and clip translation). The same pause frames also contain many `kind=1` indexed draws that this filter does not touch. Nothing in that log shows the DIPUP subset is the complete native pause list. This branch does not widen the filter, and it is **not** a visual-acceptance candidate until a later measurement shows the list is actually covered.

## Falsification (L3, before `PHYSICALLY_VERIFIED`)

1. With `PauseHeadMenu=1`, open pause: log must show `PauseHeadMenu: pooled GFx target` once, then `VR menu sink: private GFx candidate 1280x720`, and `pauseMenuProvided` > 0 in the VR counter dump.
2. **False pass:** only the VR SETTINGS row moves while the GFx list stays on the monitor (`pauseMenuRedirectDraws` > 0 but `pauseMenuProvided` == 0).
3. **False pass:** main menu / IIS / Bink / stations change with `PauseHeadMenu=0` (regression control).

## Out of scope

- OpenXR submission policy, IIS inset, VR SETTINGS row placement (landed via menu integration PR).
- Scene-sized pause capture (`CapturePauseDiegeticTarget` and similar).

## Attribution

Builds on captured-menu provider work from merged menu-flat policy (PR #5) and integration cherry-picks of PR #6 / PR #7.
