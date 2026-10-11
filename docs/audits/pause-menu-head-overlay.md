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

1. For `g_vrMenuClass == 3` only, intercept the **same** `HudStereoPair` branch as legacy menu mono (`class 2 || 3`, scene colour, not backbuffer), gated by `g_vrUiMenu`, `g_vrUiNativeReady`, and `g_gamePaused` so unrelated scene draws stay on the legacy path.
2. Redirect those draws into a **pooled `1280x720`** render target (GFx canvas size, not scene dimensions).
3. Reuse **`CaptureVrMenuTarget` + `DrawVrMenuPointer` native quad** + **`SuppressCapturedVrMenuDraw`** — same laser, stereo submission, and input mapping as main menu. Redirect only runs after successful `GetRenderTarget` / `GetViewport` / `SetRenderTarget` / `SetViewport`; failed binds restore RT and viewport before falling back to the legacy mono draw. Pool clear is keyed to `g_vfxFrame` (one clear per engine frame). `CaptureVrMenuTarget` runs only after a successful pooled draw **and** `CanProvide` for the current menu token.
4. Set `PauseHeadMenu=0` to restore legacy diegetic-only pause instantly.

## Falsification (L3, before `PHYSICALLY_VERIFIED`)

1. With `PauseHeadMenu=1`, open pause: log must show `PauseHeadMenu: pooled GFx target` once, then `VR menu sink: private GFx candidate 1280x720`, and `pauseMenuProvided` > 0 in the VR counter dump.
2. **False pass:** only the VR SETTINGS row moves while the GFx list stays on the monitor (`pauseMenuRedirectDraws` > 0 but `pauseMenuProvided` == 0).
3. **False pass:** main menu / IIS / Bink / stations change with `PauseHeadMenu=0` (regression control).

## Out of scope

- OpenXR submission policy, IIS inset, VR SETTINGS row placement (landed via menu integration PR).
- Scene-sized pause capture (`CapturePauseDiegeticTarget` and similar).

## Attribution

Builds on captured-menu provider work from merged menu-flat policy (PR #5) and integration cherry-picks of PR #6 / PR #7.
