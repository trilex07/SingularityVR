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

## RE round 1 verdict (2026-10-15 session): NO plain int32 mag on the weapon

8 pulls, same object (ARMachineGun/RvWeaponShared): no word decrements by 1;
volatile words are flags/timers/coords/event latches. Static small ints
(3/4/20/30/...) never move. Falsifier for round 2 stands: enumerate NAMES.

## RE round 5: pawn diffs + shots-fired proof (this change)

Three sessions prove no mag int on weapon/firemode actors. Remaining doubt:
did the taps actually fire? `mShotCount` (dump-named, resolved BY NAME per
class) is logged per pull — climbs iff shots fire. Pawn words diff per pull
(location sway wobbles floats; mag/reserve would show exact small-int
steps). If shots fire and nothing steps anywhere, ammo lives in bins/
inventory (mUsesAmmoType trail) — next experiment.

Dump vocabulary proves RvWeaponShared's own data holds no ammo counter, but
`+0x023C WeaponFireTypes` exists (UT pattern: live counters on the firemode
object). `WeaponFireTypes` registered in g_wantNames, resolved BY NAME per
weapon class (never assumed); `WeaponFireTypes[0]` dumped [0,0x200)
baseline+diff with `fm` tags. Falsifier: an int there decrementing once per
spaced tap IS the mag. Reserve still open (bins hypothesis).

## Runtime plan (after offsets harden like Health run-199)

Round-1/2 verdict: no ±1 int in weapon [0,0x400); class vocabulary shows
ammo lives behind GetAmmoInClip/GetAmmoInBin-style accessors with no plain
data field in the dump. Round 3 widens the weapon window to [0,0x1000) and
adds a one-shot pawn baseline [0,0x800) for reserve candidates by value.
Script discipline matters: SINGLE taps spaced ~2 s (auto spray + reloads
mask the sawtooth). Falsifier unchanged: a per-tap decrementing int IS the
mag; reserve identified by value, verified by pickup later.

`LogEquippedClass` dumps the equipped weapon's class chain once per class
(max 4) while the probe flag is armed — run-235 pattern, no guessing. Same
session also re-collects trigger diffs. Offline analysis then names mag
(+reserve) candidates that round 3 verifies by motion.

## Runtime plan (after offsets harden like Health run-199)

- Per held gun: plate quad semi-transparent behind the gun; hidden while R3
  wheel is open (no mutual overlap).
- Live digits: cover baked digit bands with bg quads, stamp strip glyphs
  (mag + reserve). White mag icon normally; red box + red mag on empty.
- `[Render] AmmoPlateEnabled=0` default OFF with lease for L3.
- Atlas: digit cells + per-gun mag/mag-red cells (keyed extraction, same
  machinery as gun silhouettes / health segments).
