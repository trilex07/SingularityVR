# Left-glove health bar

Reference mockup: `references/Futurystyczny pasek zdrowia HUD.png` (plate
2172x724, 11 segment slots, 10 filled + 1 empty frame, plus icon baked in).

## OBSERVE

- HP source exists: `Pawn.Health/HealthMax` offsets resolve at boot
  (run-199 chain); `HealWouldDoSomething` shows the guarded read pattern
  (`FindPlayerPawn` + `Readable`). Per-frame pawn walks on the render thread
  are off the table: sample on input at ~8 Hz into shared LONGs.
- Hands: physical left is `g_handLastPos[0]` (index 1 is right;
  `AimHand()` only swaps roles). Head pos `g_headPosXR`, eye basis
  `g_eyePose[0]` + `g_targetTanX/Y` (same reads the wheel hand-select uses).
- Two-hand latch exists: `g_twoHandActive` (support grip, input thread).
- Pack already ships `plates/health.svg` + `plate_health.glb` (SIDE design);
  the runtime atlas had no bar cells until this change.

## HYPOTHESIS

A glanceable plate on the left glove (face-raise to look, or riding above
the left hand while two-handing) with segment count from sampled HP proves
the feature without touching native HUD or gameplay. Falsifier: L1 gesture
failure, L2 build failure, or an L3 session where the bar shows with a wrong
count vs sampled HP, appears outside the gesture, or flickers at the
boundary (hysteresis 40/55 cm covers the last).

## SCOPE (art + gesture, this change)

- `spikes/view_matrix/health_gesture.h` (new, pure) + `tests/test-health-gesture.cpp`
  (18 checks PASS): `FaceRaiseShow/Hide` (40 cm show, 55 cm hide, in-front +
  above-waist gates), `FilledSegments` (11 slots, clamps, NaN/unknown -> -1,
  never fabricate), segment count 11 per mockup measurement.
- Plates atlas (installed data, Steam only):
  `vr-assets/hud/katorga/plates-atlas/katorga-plates-atlas.png` (512x256) +
  `plates.json` (`hud_atlas`-compatible: `plate-health-bg` 384x64 at 0,0;
  `seg-full` 64x114 at 384,0; `seg-empty` 64x114 at 448,0). Fresh install
  (no prior files); PNG sha recorded in JSON and verified.
- Extraction: tight crops from the mockup, plate-bg corner-median key,
  white-free (original blue kept), bicubic into cells; sources kept at
  `vr-mods/references/plate-{health-bg,seg-full,seg-empty}.png` (outside repo).
- Runtime slot fractions (of plate 2001x330 @ (84,183)): slot i x0 =
  0.1589 + i*0.0714, width 0.0715; y 0.191..0.961. For the renderer (next).

## OUT OF SCOPE (next change)

Runtime: HP sampling + visibility state on input, `DrawHealthPlate` on
render (bg quad + 11 stamped seg cells), `[Render] HealthBarEnabled=0`
default OFF with lease for L3, L2 build, deploy, headset placement tuning
(face-raise vs two-hand offsets). AMMO-style fabrication ban applies to
segments: unknown HP hides the bar, never a guessed full one.

## RUNTIME (this change)

- `DrawHealthPlate` after the squares wheel in the copy path (same BeginScene block, state block, per-eye projection); physical left hand anchor (+8 cm face mode, +12 cm two-hand), plate width 18% frame height, bg + 11 stamped seg cells, white tint, alpha blend. Accepted run-217 band untouched.
- HP from the cached pawn on the render thread (no scans; Readable guards); unknown HP hides the bar with a one-shot log. Visibility: face-raise hysteresis (`g_hbShown`, render thread) OR two-hand + weapon in hands. Bounded logs: atlas ready, shown edges with hp/max.
- `MaybeLoadPlatesAtlas`: lazy WIC BGRA decode, dims verified vs JSON, MANAGED texture; any failure hides the plate for the session. `[Render] HealthPlateEnabled=0` default OFF + `KatorgaPlatesAssetRoot` path key. build.ps1 links windowscodecs/ole32.
- Left-handed mode caveat: physical left hand (index 0) is the anchor regardless of AimHand mirror.
