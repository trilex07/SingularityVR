# Round weapon-wheel layout — ring + wedge sectors + catalog labels

Reference mockup: `references/95f791be-39f6-461b-8470-8ff0664ee7e6.png`
(6-wedge circular wheel, per-sector label/icon/ammo, orange selected wedge,
centre SELECT WEAPON).

## OBSERVE (2026-10-15, PID428 session `ww-visual-20261015-0032`)

- Catalog ready (8 sectors, 4 cycle entries), atlas ready (512x256, 16 icons),
  overlay active with Katorga icons, 4 sectors, anchored to the right hand.
- The log contains repeated `OPEN slots=4` and **zero** `COMMIT` /
  `SelectionChanged` / `LANDED` lines: the user opened the wheel but never
  committed a selection (stick inside deadzone, release on sector 0, or wheel
  opened without a weapon in hands — `Update()` requires
  `gameplay && weaponInHands`).
- User report: top shows a circle icon, left/right/down show squares; asks
  whether selection is right-hand movement. Answer from
  `spikes/view_matrix/weapon_wheel.h`: **selection is the RIGHT STICK
  direction** (`SectorForStick`, deadzone 0.35, sector 0 = up, clockwise);
  the wheel is only *anchored* to the right-hand pose. R3 hold opens,
  stick tilt highlights, R3 release commits (`selection > 0`), L3 cancels,
  release on sector 0 is Cancel by design (no-op "current" sector).
- Current visuals are floating squares (`W1..W4` or 64px atlas quads showing
  magazine/sense glyphs) — no ring, no wedge sectors, no weapon names. The
  installed atlas has **no weapon silhouettes**, so reference-style gun
  pictures are not renderable from it.

## HYPOTHESIS

Replacing square cards with wedge fills + outer ring + centre disc + catalog
labels (same input FSM, same commit path) proves the circular layout without
changing what the wheel *does*. Falsifier: L1 geometry round-trip failure,
L2 build/test failure, or an L3 session where R3 no longer opens the wheel
or COMMIT pulses change count vs the squares build.

## SCOPE (this step)

- `weapon_wheel_layout.h` (new, pure): sector angles, wedge bounds/points,
  `RoundMetrics` (wheel diameter ~40% frame height), glyph-set label
  sanitation, `LabelForSlot` with slot-0-shows-held-weapon.
- `weapon_wheel_catalog_load.h`: parses `label_pl` (was discarded).
- `d3d9.cpp` `DrawWeaponWheel`: round path behind `[Input] WeaponWheelRound=0`
  (default OFF — squares path byte-identical when OFF). Slot-0 icon also
  shows the held weapon when the catalog resolves it. FSM untouched.
- Centre caption `SELECT` / `WEAPON`. Per-sector labels from the catalog,
  `W#` fallback when the catalog is absent, icon-only when a label is empty.
- OUT of scope: ammo counts (AMMO_UNKNOWN — no native ammo hook proven, never
  fabricate), weapon silhouettes (not in the atlas), 3D ring/plates GLBs,
  sector-0 commit semantics (still Cancel/no-op), 6-slot default (slots stay
  `WeaponWheelSlots=4` until Seeker/TMD sectors get verified `gun_verts`).

## EVIDENCE GATES

- L1: `test-weapon-wheel-layout` (sector/stick round-trip 2..8 slots, wedge
  bounds, metrics, sanitation incl. `/` mockup labels, slot labels incl.
  held-weapon override/truncation) + existing wheel/catalog/atlas tests.
- L2: x86 PE32 `/O2` DLL build, SHA recorded; tests above re-run.
- L3 (MAIN only): leased INI `WeaponWheelRound=1` (+ existing
  `WeaponWheelDirectSelect=1`, `KatorgaHudEnabled=1`), R3 open shows the ring
  with 4 labelled wedges, tilt highlights orange, release commits exactly as
  the squares build; then INI restore + DLL rollback, hash-verified.
- L4: physical PSVR2 readability/placement acceptance (separate).

## LIMITS

- Labels use the 5x7 glyph set (A-Z 0-9 `+-()` space); `/` and `.` render as
  space explicitly, Polish diacritics are not in the font. 16-glyph cap.
- Long labels (e.g. `KASIMOV SNV-E99` = 15) fit 4-slot wedges at px=2; 6+ slot
  wedges may see minor text overspill — noted, not blocked.
- Catalog/atlas reads on the render thread follow the existing pattern
  (written once at first R3 open); same accepted lifetime as the icon path.
