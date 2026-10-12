# Pause mono capture — blocker (2026-10-12)

No renderer change. The installed candidate stays `fix/pause-menu-modes` @ `96390ab`
(DLL `2891A9D873BF228AB0DD6A89CA63AB2B67F648ADA115E1383A9C54F2902634F2`).

This agrees with `research/menu-intro-rootcause` @ `d8c4c95`,
`docs/audits/pause-menu-stereo-rootcause-20261012.md`.

## What the pause-session log actually did

Source: `%LOCALAPPDATA%\SingularityVR\view_matrix.log` from the `96390ab` headset session.

**Submit (one record):** `pause-evidence submit: class=3 dup=1 stereo=0 menuFlat=0 cropEyes=0 backbuffer=5264x2684`.
Each eye still receives one half of that image.

**Class-3 batches that reached `HudStereoPair` (counts for the session):**

| Batches | Target | compose | c5 |
|---|---|---|---|
| 5878 | offscreen 5264×2684 | 0 | `sx=1` (not a GFx canvas) |
| 4594 | backbuffer 5264×2684 | 1 | `sx=7.812e-05`, `ty=0.5945` |
| 1284 | backbuffer 5264×2684 | 1 | `sx=1.249e-05`, `ty=0.5557` |

The two small-`sx` groups are the GFx canvas orthos (main rows and the second column, including Objectives). They are drawn on the **backbuffer**, then the OpenXR bridge splits that buffer. The offscreen `sx=1` draws are not that movie.

## Why each mode looked like a failure

- **LEGACY:** native path. The complete layout spans the 5264-wide backbuffer; submission keeps one half per eye.
- **HEAD:** redirect runs only for class 3 on a **non-backbuffer** scene target (`GfxPauseMenuMonoBatch` requires `rtIsBackbuffer == false`). The canvas batches above never enter it. A 1280×720 allocation is not those batches. HEAD also skips redirect while the VR Settings panel is open (`panelOpen`). That gate did not cause this miss: the canvas draws are on the backbuffer either way, and the log keeps recording them across mode changes.
- **FULLGFX:** after `selected=2`, the log shows `pooled GFx target 1280x720 ready` and `VR menu sink: private GFx candidate 1280x720`. That sink is the stretched **offscreen scene** (`sx=1`), not the backbuffer canvas. The canvas lines continue immediately afterward, still on the backbuffer, still unsuppressed. There is no `mode=2 src=scene` success line for a UI-only surface.
- **FULLSRC:** `pause-mode: mode=3 src=backbuffer 5264x2684 present=identical halves` did run. It copies the whole side-by-side frame into each eye, so each eye shows two screens. That matches the headset and is not a UI capture.

## Blocker

There is no private class-3 movie texture that already contains both the main pause UI and Objectives.

Those batches share the 5264×2684 backbuffer with the stereo world. The only image that contains every column is that full frame. Copying it (FULLSRC) or stretching the offscreen scene (FULLGFX) cannot isolate the UI. Redirecting the offscreen path (HEAD) never sees the canvas batches.

A mono replacement would require a proven composition of **those backbuffer canvas batches** into one private surface, then the existing head-plane provider, and only then suppression of the native draws. That surface is not proven. No further c5, half-viewport, scissor, or whole-SBS copy is justified from this log.

## Left installed

Same DLL and INI as `96390ab`. Mode selector and LEGACY fallback unchanged. Gloves, grip, holster, Polish, and the texture injector were not touched.
