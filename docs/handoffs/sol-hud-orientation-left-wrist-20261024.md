# HUD reader orientation and left-wrist evidence

Branch: `fix/sol-diegetic-hud-native-projection`, follows `d79ad975`.
SIDE only: no installed files changed, no game launch or Steam deployment.

## HUD: confirmed cause and correction

Physical feedback confirms that the counter now follows the weapon, but reads
backwards. The body/camera snapshots in the same session prove the orientation:
all six successfully projected recorded poses have negative screen-X for the
fixture's +X axis and negative screen-Y for its +forward/top axis. In the first
Volk sample the normalized derivatives are right=(-0.0150,+0.0026),
top=(-0.0061,-0.0378). Thus the horizontal axis is reversed; the vertical axis
is already correct. This is a reflection, not a 180-degree rotation.

`BuildFixtureQuads3D` now uses readerRight=-modelRight for the bezel, screen and
glyphs, INCLUDING their advance/order. Reversing only each glyph's UV would
leave the number sequence backwards. Housing bounds, depth, pitch, Y reflection,
native pose/camera capture, ammunition sampling and fallback remain unchanged.
No head-facing billboard or automatic rotation is added: the display stays
attached to the weapon at arbitrary rolls.

The old synthetic camera looked from the opposite horizontally oriented side;
it could prove visible pixels but missed this reflection. The fixture camera
now matches native handedness. Six observed poses from the failing session are
checked in `tests/diegetic_observed_poses.h` (source log SHA256 included).

Falsifier/regression: increasing U and successive glyphs must move right in
each recorded eye; increasing V must move down. The uncorrected geometry fails.
Synthetic roll cases remain attached rather than rotating to face the viewer.

## Left hand: NOT fixed yet, bounded missing measurement supplied

The HUD correction and its predecessor do not write controller positions,
independent-hand palettes or TMD placement. The screenshot cannot determine
the physical pad position. Do not treat temporal coincidence as proof that the
HUD caused the offset.

Current session: HandAnchorProbe=0, NoTmdWristAnchorFix=1, GloveLeftShift=12,
TmdOnOffHand=1, TmdShortGauntlet route executes. The short-gauntlet material-0
path already bypasses GloveLeftShift. Removing that setting globally would not
fix this path. Existing final-bound receipts predominantly measure RIGHT wrist
51; the earlier LEFT assembly receipt has no synchronized pad target/local/ride
chain. Those records do not establish the left-hand positional residual.

The code uses a gun-anchor-based controller transform for the TMD left wrist,
whereas the noTMD free-wrist path reconstructs the actual wrist. This is a
specific remaining hypothesis, NOT a proven physical cause: native pose and
support ownership also matter. No offset or TMD transformation is changed.

The existing `[Render] HandAnchorProbe=1` now additionally samples exact TMD
material 0 at final-bound stereo submission, before IB/palette restoration:

- eight records per eye, at least 2 s apart; failed attempts also bounded;
- original bind bones 9/51 and audited palette slots 1/19;
- bound IB, selected face count, hands/support, both skin matrices, LocalToWorld,
  actual camera ride, controller transforms and synchronized target positions;
- reconstructed wrist world coordinates and error against each pad in UU;
  UU/metre recorded, allowing conversion to centimetres;
- collection performs no pose writes or GPU readback; flushing is in Present;
- default OFF; noTMD parser explicitly excludes TMD records to avoid replaying
  them using the wrong bind skeleton.

MAIN expected receipt: `tmd-hand-anchor final-bound: ... eye=0/1 ... errorL=...`.
This is bound-state evidence, not proof of pixels or physical placement. During
support a nonzero pad/wrist residual can be intentional (authored weapon grip).
Compare support=0 first. A stable native-wrist residual versus pad validates the
anchor hypothesis; a zero residual with displaced glove redirects investigation
to geometry/grip-pose reference or tracking-space origin. Do not fix both with
one guessed offset.

## Verification

- `tools/hud/test-diegetic-hud.ps1`: L1 geometry/atlas/freshness PASS;
  24 synthetic stereo projections plus 6 recorded PSVR2 pose replays PASS;
  120 D3D9 + 120 D3D9Ex actual-atlas pixel cases PASS (including observed poses,
  hostile state/restoration, failure paths and Reset).
- `test-hand-anchor-probe.cpp`: bounded stereo metadata, invalid inputs and
  separate budgets PASS. `test-hand-anchor-math.cpp`: original bind/world/ride
  reconstruction and preserved transforms PASS.
- `tools/tests/test-hand-anchor-log.py`: seven parser regressions PASS.
- `tools/tests/test-dual-weapon-tmd.ps1`: hand/support, TMD, reload, holster,
  carried magazine, measured aim, WeaponLocker and OLED CPU/D3D11 suites PASS.
- x86 PE32 `/O2` build PASS, clean source commit
  `36081c3dff1529580ad98e8ec9fb2a5e41c25152`. DLL SHA256:
  `857F498D7142E444C089AE35E596BD1F3C7E8BB41844AEE14569C765572C1337`.
  HUD-only commit: `5181cdc4`; diagnostic follow-up: `36081c3d`.

## One MAIN-owned acceptance session

Keep current accepted configuration/assets and enable HandAnchorProbe only for
this measurement. Inspect the counter on Volk and Valkyrie: clip on the LEFT,
reserve on the RIGHT, readable digits/slash, unchanged attachment when rolling
the weapon. Centurion shares the readout fix but has no captured session here.
During the first 15 seconds after gameplay appears, move the free left hand near
and away from the face, rotate the wrist, then briefly hold/release support Grip.
No chat switching required. Collect the log and physical hand-offset result.

GO for review/controlled HUD orientation validation and wrist measurement.
NOT a claim that left-hand tracking is repaired. No physical PASS claimed for
the new correction.
