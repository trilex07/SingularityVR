# Ammo plates (Katorga, per gun)

Reference art (`vr-mods/references/`):
- `hud_centrurion.png` (typo in source name), `hud_rifle.png`,
  `hud_shotgun.png`, `hud_sniper.png`: full plates — mag icon, big mag
  digits, bullets + reserve digits, gun label, red empty-mag box, `>>`.
  Baked numbers are EXAMPLES (Centurion 12/48, Rifle 30/180, Shotgun 6/24,
  Sniper 5/20 — mag sizes match the reference wheel).
- `b8ed7484-....png`: digit strip 0-9 in plate font (separate live digits).
- `centurion_ammo.png`, `rifle_ammo.png`, `shotgun_ammo.png`,
  `sniper_ammo.png`: per-gun reserve bullet renders.
- Mag states: white mag = full/normal, red mag + `!` = empty (inside the red
  box). Runtime shows white normally, red zone on empty.

## RE first: ammo offsets UNKNOWN

No native mag/reserve hook is proven. Evidence-driven probe (this change):
`ammo_probe.h` gate (L1) + bounded dumper in d3d9.cpp next to the fire
census (input thread): `[Diagnostic] AmmoProbe=N` pulls, baseline snapshot
per weapon object ([0,0x400), 256 guarded reads), changed-offsets-only after,
then silent. Falsifier: an int decrementing once per spaced shot IS the mag
counter (expect start at mag size 12/30/6/5); reserve moves on pickup, not
fire — identified by value, verified later. If nothing moves on the weapon
object, widen to pawn/inventory chain (next experiment, not this one).

## Runtime plan (after offsets harden like Health run-199)

- Per held gun: plate quad semi-transparent behind the gun; hidden while R3
  wheel is open (no mutual overlap).
- Live digits: cover baked digit bands with bg quads, stamp strip glyphs
  (mag + reserve). White mag icon normally; red box + red mag on empty.
- `[Render] AmmoPlateEnabled=0` default OFF with lease for L3.
- Atlas: digit cells + per-gun mag/mag-red cells (keyed extraction, same
  machinery as gun silhouettes / health segments).
