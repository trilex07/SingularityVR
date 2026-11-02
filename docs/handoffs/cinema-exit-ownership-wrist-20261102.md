# CBF16F93 follow-up — empty hands, ownership and cinema exit

Branch: `fix/cinema-exit-ownership-wrist`, based on `9ab0ad88`.

## Physical evidence and acceptance scope

User reports full cinema framing/edges OK (preserved); empty-hand wrist HUD
clips while the pistol presentation is OK; scripted mutant hit succeeds only
occasionally. Requested cinema-exit recenter and ownership-gated R3 wheel.
These reports do not accept the mutant fix or the empty-hand HUD.
Installed preimage remains CBF16F9396740F6F9B98118CE5726F6646085AE0A969AAE68928906618222C8D.

## Corrections

- R3 open and target selection now require a native possession mask for the
  current pawn, or a Seeker availability receipt belonging to that pawn. Model
  catalog/previously observed meshes cannot authorize selection. An unknown
  catalog cannot produce guessed native cycle pulses. Native selector still
  re-snapshots ownership on the engine input thread before selecting an item.
- CutsceneFlat -> GameplayStereo requests the existing L3+R3 common-stage
  recenter exactly once. No synthetic button input or separate HMD/hand offset.
  Menu/pause/startup transitions do not request recenter. Accepted cinema
  framing, stereo matrices and eye cropping are unchanged.
- Empty-handed noTMD gloves can teach the existing retained-buffer depth-prime
  cache without a weapon object. Only exact 3314/2924, first/base/min=0,
  4830-triangle candidate ranges qualify; omission still requires identical
  retained VB/IB/stream/range, <=2-frame successful restored stereo color,
  verified native prime viewport, scene target, depth write, color=0 and
  stencil=0. No pose/mesh offset, new depth copy or broad suppression.

## HUD evidence and falsifier

`build/cinema-framing/session-CBF16F93/view_matrix.log` has the one-shot final
GPU palette and actual-bound SVNC receipts. Weighted reconstruction gives
+0.92922293 UU conservative clearance in BOTH eyes, not mesh intersection.
The depth trace shows early moved glove draws 1/2 in world DS 387FF4C0 with
viewport MaxZ=.001, before world draws and the late foreground depth redirection.
The old cache explicitly rejects weapon=0 and its owner context also excludes
holstered presentation. This leaves obsolete near-plane hand depth in the
preserved world depth, a mechanism matching the empty/pistol difference.

This is an evidence-supported correction, NOT physical proof: if the live
retained source siblings/state fail to qualify, the cache still refuses. Expect
`tracked-depth-prime: learned restored color sibling ... verts=3314/2924 ...`
and `omitted` receipts. If the HUD still clips with omission confirmed, the
prime hypothesis is falsified; inspect actual depth projection instead. The
unproven draw-local camera replacement was discarded before delivery.

## Mutant: specific remaining dependency, NOT FIXED

CBF16F93 has no `cinema native shot` receipt: the old observer required the
FireAmmunition window, which the reported scripted encounter did not enter.
Read-only package inspection confirms RvCH_ZekGrabHandler -> RvCombatHandler ->
Fightstyles.CombatHandler, with native behavior and no exported aim/target
function on that handler. Handler update parameters expose mPawn but do not
prove an attacker target. Do not invent a pointer, hit result or global auto-hit.

The candidate adds at most 24 distinct native SingleLineCheck call-site/source/
flags receipts under cinematic R2, independent of FireAmmunition. These record
actual source class, caller, range and fireDepth, explicitly **not a confirmed
shot**. No trace parameters/damage/ammo are changed. Needed discriminator: does
this encounter issue a local shot ray, a handler/other-source ray, or no SLC ray?
Then inspect that exact native caller. The current build does NOT claim to
implement reliable scripted auto-hit.

## Validation

- x86 real R3 Update tests: new-game blocked, owned pickup opens, unowned sniper/
  Seeker refused, lost ownership cancels. Cinema transition matrix PASS.
- D3D9Ex actual retained-buffer/state/reset tests PASS, including real synthetic
  3314-vertex/4830-triangle empty-hand source: preserved-world HUD pixel black ->
  FF00FF in BOTH eyes. Old weapon prime/LEQUAL/EQUAL fixtures still PASS.
- x86/x64 gaze/cinema/damage policies and D3D9Ex all-eight-corner pixels PASS.
- Dual-TMD/hands/support/reload/WeaponLocker/OLED regression suite PASS.
- Legacy D3D9 HAL is NOT RUN: CreateDevice fails on this host (8876086A/6C).
  Explicit --ex-only runs the original and new fixture bodies through D3D9Ex;
  it is not reported as legacy D3D9 PASS.
- HUD policy/projection/source/depth/placement/integration regressions: see local
  build/cinema-framing/hud-regression-current.log; GPU legacy boundary separate.
- Optimized x86 PE32 build identity recorded in staged d3d9.build.json after
  clean commit. New real Steam runtime and PSVR2 verification PENDING.

## One grouped acceptance

1. New game before a gun: R3 cannot open/select unowned guns.
2. Inspect inward empty left wrist, then after legitimate pistol pickup: HUD
   should remain outside the glove and be occluded normally.
3. Exit helicopter/scripted cinema: common stage recenters once; pause does not.
4. Mutant R2 firing records the bounded native trace receipt. MAIN reads it;
   do not ask the user to inspect logs or call this a fixed hit test.

No Steam changes or launch in this implementation turn. Deploy only after the
fresh start gate, closed-game backup/full profile and built/deployed hash match.
