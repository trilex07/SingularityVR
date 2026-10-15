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

## RE round 9 verdict: HUD cache PROVEN live

`mClipAmmo`/`mBinAmmo`/`mClipSize` (+0x578/57C/580 on RvHUD): clip
4→3→2→1→[reload]→6→5→4 (-1/tap), bin 16→10 on the -6 reload. History:
rounds 1-5 ruled out weapon [0,0x1000), firemode, pawn ints (+0x114 pawn
clock counts shots); vocabularies hold no ammo data fields; InvManager
words static; HUD dump needed forced find + relocation to fire at all.

## RE round 10: plates runtime (this change)

Per-gun plate bg + cover quads over baked digits + right-aligned strip
glyphs (mag 3, reserve 4 digits) + white-mag cover iff empty; hidden on R3,
on unknown gun/ammo, without weapon. `[Render] AmmoPlateEnabled=0` default
OFF. Anchor: aim hand +12 cm; width 24% frame height (placement tuning left
for L3). Art: 4 plates (1024x341) + digits strip (2172x240) in
vr-assets/hud/katorga/ammo/ (installed data).

- Per held gun: plate quad semi-transparent behind the gun; hidden while R3
  wheel is open (no mutual overlap).
- Live digits: cover baked digit bands with bg quads, stamp strip glyphs
  (mag + reserve). White mag icon normally; red box + red mag on empty.
- `[Render] AmmoPlateEnabled=0` default OFF with lease for L3.
- Atlas: digit cells + per-gun mag/mag-red cells (keyed extraction, same
  machinery as gun silhouettes / health segments).
