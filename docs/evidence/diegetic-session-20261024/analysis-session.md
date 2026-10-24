# Diegetic-session evidence (Centurion candidate, pre-pivot) — 2026-10-24

## What this archive is

Complete `%LOCALAPPDATA%\SingularityVR\view_matrix.log` of the user's run that
started `2026-10-24 11:43:39` on the *Centurion-only* diegetic candidate
(dll `ADF2833A70E0BE027AE4028134B5BFD2CD86759005A7D00AFD7112803B2625DF`,
INI `257F9DFB14A4...`, deployed 11:25). The file on disk is byte-identical to
this archive (`2422728` bytes, 18354 lines; no further writes after the
`SetPause -> PAUSED` tail). The run was ended by the 11:55 guarded
`-StopGame` deployment step of milestone 2. Local-only per the `*.log`
gitignore rule; the statements below are the committed record.

sha256 (live at archive time): `246E8B496515`… (full hash verified at copy
time; size and line count re-verified at re-archive).

## Receipts used for the milestone-2 pivot

- `diegetic hud: ENABLED (Centurion counter, default off, atlas
  vr-assets/hud/diegetic)` — the gate parsed and armed in the real game
  (L3 evidence for INI parse + startup logging).
- ZERO `diegetic hud:` draw lines and ZERO atlas lines. The atlas loader is
  lazy (first mounted-gun draw, `d3d9.cpp` ~L31285) and the user never held
  the Centurion — the log is exactly consistent with "Centurion not owned".
  Consequence: the diegetic atlas raster path had **no physical exercise**
  in this session; milestone 2 (Valkyrie/Spikeshot mounts) is its first.

## Accepted-pipeline receipts (regression watch)

- `ammo plate: ENABLED (ammo root vr-assets\hud\katorga\ammo)`
- `ammo plate: art ready (4 plates + digits)`
- `ammo plate: ammo unknown (clip=-1 bin=-1 hud=00000000 offC=-2 offB=-2) -
  hidden until readable` — correct hidden-until-known behaviour.
- `ammo plate: HUD cache clip+1404 bin+1400 size+1408 on RvHUD` — the native
  HUD offset probe resolved live onto `RvHUD`.
- `ammo plate: drew plate=2 clip=6 bin=59 705x235 at 2468,2837 (backbuffer
  5640x2876)` — the user held the Spikeshot (plate 2 per the accepted
  ammo_layout mapping) and the accepted plate rendered **live** ammo
  (clip=6, reserve=59) inside the accepted stereo backbuffer. This is direct
  runtime evidence that the plates pipeline is intact on the Centurion
  candidate, and that the Spikeshot is present in this save with real values
  the diegetic counter must match (note: live reserve 59 vs fixture '32' —
  the fixture readout was an inventory snapshot; the display tracks the
  native sampler, not the fixture).
- `menu mono: class=2 one draw into the diegetic target`, `handsymmetry ...,
  valid 1/0`, `SetPause arg raw 0x00000001 -> PAUSED YES` — expected accepted
  behaviours, no mischief lines / failure scan hits reported.

## Decision recorded

Milestone 2 = extend the measured mounts to Valkyrie `11387` + Spikeshot
`8297` on the same branch; Centurion implementation preserved for a future
save. One controlled deployed change: the renderer dll
(`B5F2A2AD4ADB...` @ `bc4c32147c3b`); INI byte-identical (359 keys, gate
`DiegeticWeaponHud=1` armed); rollback backup
`deployment/install-backups/20261024-115514` = the Centurion candidate pair.
