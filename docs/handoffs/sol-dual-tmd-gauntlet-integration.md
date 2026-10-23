# Dual-weapon TMD + short-gauntlet integration — SIDE handoff

Base `d771c40` (Sol `7e7d8208` + Astra `ac07d16`), branch
`fix/sol-dual-tmd-gauntlet-integration`. No Steam/game installation touched.

## Observed integration conflict

The 2026-10-23 10:45:46 Steam log (read-only) confirms both features enabled,
the exact TMD mixed section (`min=2626`, `verts=4818`, `first=13206`,
`tris=7704`), and a successful two-eye SVTG draw of 7110 triangles with
index/palette restoration. Thus neither missing SVTG data nor a wholly
unreachable short-gauntlet dispatch explains the reported screenshot.

The dual census at frame 4950 records 900 TMD-hardware draws in
`HANDS_WEAPON` with `gunRide=1`. `RideTargetFor(verts)` lacked material-range
identity: once `TmdRaised()` was false it selected the right-gun camera ride
for a section whose independent palette follows the physical left wrist.
This is a proven *pose-ownership conflict*. The screenshot's exact pixel cause
remains an inference until another PSVR2 comparison; the census is signature
evidence, not pixel evidence.

Hypothesis: the right-gun ride plus left-wrist palette creates the stretched or
misplaced TMD/glove presentation during dual use. Falsifier: with the exact TMD
sections no longer assigned gun ride, the same stretched geometry remains in
the same hands/weapon/grip state. In that event capture one bounded palette and
draw identity for the failing TMD material rather than adjusting offsets.

## Correction

- Pass `startIndex` and `primCount` into `RideTargetFor` for indexed draws.
- Only in enabled dual presentation, normal non-shadow scene foreground, and
  with independent hands, give the three exact native TMD sections a single
  pose owner: their existing controller-tracked bone palette. The gun, muzzle
  FX, non-indexed draws, and standalone TMD presentation retain their prior
  route.
- Existing `FgHidden` early admission, `AfterPalette` failure suppression,
  7110-face SVTG draw, 49-run hardware fallback, two-hand hide and D3D9
  index/palette restores are unchanged. No asset or bone-map edits.

## Offline validation and L4 gate

- Short-gauntlet test with private PSK/cooked/SVTG: PASS (index identity,
  five wrist rotations, exact dispatch/fallback).
- Dual-weapon policy and actual-callsite wiring: PASS.
- TMD runtime regressions: PASS including D3D9 and D3D9Ex two-eye 7110-face
  draw/readback, WeaponLocker and OLED D3D11 tests. HAL creation failed in the
  restricted sandbox; the same offline test passed with GPU access outside it.
- x86 PE32 `/O2` build: PASS. These are L1/L2 only, not physical proof.

MAIN should compare a right-hand weapon plus left TMD in one PSVR2 session:
L2/R2; rotate the left wrist; left Grip support and release; weapon switch,
holster and reload; check both eyes and frame time. A left-controller tracking
dropout must not reveal the long native forearm. Do not promote before L4.
