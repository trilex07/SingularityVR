# Meter test — world scale calibration procedure

Measures instead of guessing. Do this once per headset; repeat after any
`WorldScalePct` change or runtime render-scale change.

## What you need

- A tape measure or a known 20 cm object (ruler, book).
- The game running with this build, VR mode ON, standing (recentered standing).

## Steps

1. Launch, recenter standing (MENU hold), stand still, wait 10 s.
2. From `%LOCALAPPDATA%\SingularityVR\view_matrix.log`, copy the latest
   `scalechain:` line. Note `UU/m` (expect ~52.5 at `WorldScalePct=100`)
   and the `eyeDelta ... mm` value (expect ~63 mm for a typical IPD).
3. Move your HEAD exactly 20 cm sideways (measure against a wall/doorframe).
   Read the next `scalechain:` line: `handOff` is hand-relative, so instead
   watch the `6dof:` line — its offset delta must read ~10.5 UU at 100%.
4. Hold both controllers together, pointing forward. Read `handsymmetry:`:
   aim and off yaw/pitch must agree within ~1 degree.
5. Point both controllers at one wall mark. The bullet must land where the
   AIM hand points (aim mode 11). If it lands with the head, the trace path
   is at fault, not scale.

## Reading the results

| Observation | Meaning | Action |
|---|---|---|
| 20 cm head move reads ~10.5 UU | world scale correct | do NOT touch `WorldScalePct`; suspect FOV or mesh calibration |
| reads ~2x off uniformly (head, hands, eyeDelta) | `MetresToUU` wrong for this runtime | change `WorldScalePct` by the measured ratio |
| only hands/gun look big, head motion correct | foreground mesh calibration, not world scale | gun anchor / mesh path, never `WorldScalePct` |
| monsters big but head motion correct | FOV or eye-height perception | check FOV readback vs headset, recentre standing |
| aim/off dev disagree >2 deg, hands together | conversion asymmetry | report the `handsymmetry:` lines |
| eyeDelta reads 0.0 mm | no positional tracking or mono fallback | check `posValid`/parallax lines first |

## Rules

- Never change `WorldScalePct` on feel alone. One measured ratio beats ten
  guesses; a wrong global scale corrupts HMD, hands AND eye baseline at once
  (all three consume the same `MetresToUU()`).
- World scale and gun/mesh size are separate knobs. `WorldScalePct` moves the
  world; gun anchor keys move the gun. Do not fix a big gun with world scale.
