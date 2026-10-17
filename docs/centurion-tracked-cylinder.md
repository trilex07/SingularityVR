# Centurion second physical feedback — 2026-10-17

SIDE, zero launches/deployment/installed-file writes. User rejects rightward
opening, travel on tilt, invisible/unusable shoulder ammo and gun drifting when
empty. New decision: Circle OPENS the cylinder; tilt only dumps the cases.

## Evidence before intervention

Read-only snapshot of the actual last session, header12:07:36, ending12:09:57:
`build/centurion-reload/feedback-20261017/session-1209.log`, SHA-256
`E385B6E7C9A69456D6ECBC1BE0A06CA5FB062AD44B965C248B1E87363EAF9906`.
VisualFollowup=1. HUD reseat at line5735 changed the zombie instance; later
ammo reads are valid. Circle/open/dump actually ran. The log repeatedly reports
grab-fresh32 times behind physical LEFT shoulder (5.5–12.9cm), then discarded32 times at
38.5–100cm from the AUTHORED controller socket. No successful insertion shown.
Thus "no grab" is not established: the input DID grab, whereas rendered carried
content and physical fit are not established by the draw counter.

Structural issues in the tested source:
- Circle enters Unlocked; lateral/roll gesture is still required before dump.
- Frame12 native PSA translation chooses its own model-space side; its small
  translation norm does NOT prove physical-controller-left opening.
- RootHold freezes only the model-space parent skin matrix. The shader still
  multiplies the CURRENT LocalToWorld and BuildGunC; that composite is NOT held
  relative to the controller. A changing native parent/component/view can move
  the rendered gun while the input socket remains a separate authored offset.
- The old socket is controller +(-35,-35,-80)mm, NOT the rendered open joint.

## Hypothesis and smallest reversible experiment

Separate default-OFF `CenturionTrackedCylinder` (requires VisualFollowup and
CylinderAnimation). Capture the original gun parent relative to the tracked aim
controller through the FULL existing shader chain once per transaction; invert
the CURRENT chain every draw. Hold the parent/controller relationship, not a
matrix in an unrelated space. Preserve native ammo/backend and replay material.
Use a compact physical-left translation for the original rigid clip, scaled by
the measured original clip width5.284UU plus clearance (5.6UU total); retain
original mesh/UV/material/size, do not mirror the mesh or guess PSA axis signs.
Publish the actual rendered open clip joint back as a bounded fresh
pawn/weapon/transaction-bound insertion witness. No witness => no insertion or
shell launch from a guessed point. Do not inflate the shoulder/insert radii.

Falsifiers: full root/controller affine does not roundtrip while native
LocalToWorld changes; clip shift is not controller-left at yaw/pitch/roll; a
stale/foreign socket completes insertion; original gun-child palettes leak;
Circle still needs an opening flick; original OFF behavior changes. Real game
lighting, clip fit/visibility, glove relationship, no drift and insertion remain
MAIN L3/PSVR2 L4 questions, not inferred from offline math.

## Implemented / handoff

Source `0742736a2c5c127f35b7c67b48bbdb8101e73a7d` on
`fix/centurion-visual-feedback`; draft PR48, not merged. MAIN's current HUD
reseat fix through2a596f9/e079e36 was synced first and its regression preserved.
MAIN can apply implementation0742736 onto its current integration, rather than
deploying an older SIDE DLL or reapplying the whole history. The PR remains
stacked on the older grip integration; source delta is the implementation
commit, NOT a claim that an older PR base is today's accepted installation.

New control opt-in is `[Input] CenturionTrackedCylinder=1`, default0, explicitly
requires existing VisualFollowup/CylinderAnimation. Keep all other tested flags,
private caches, INI calibration and accepted dependencies. No new assets and no
installed-file changes. OFF retains the previous controls/visual path.

The FULL root chain is now controller-relative through CURRENT LocalToWorld,
BuildGunC and TrackingMap, rather than holding an isolated model-space matrix.
Hammer/trigger bones also compensate current native parent motion. The native
rigid clip translates LEFT by5.6UU in the tracked aim frame (scale applied ONCE),
without mirroring vertices/UVs or using the native extraction excursion.
Loaded holds it open until the closing flick; close is280ms. Native UE3 reload
notifies/ammo still execute; their animation state is not patched or faked.

Insertion witness is published only AFTER successful native stereo draw and
palette/index restoration, protected by a nonblocking lock, identity/transaction
binding and100ms lifetime. Input tests the SAME carried mesh-centre offset, not
only the left controller origin, against the actual open clip joint. Shoulder
radius13cm and insertion radius16cm are unchanged. No fresh socket means no
insertion permit and no shell launch from a guessed point. Snapshot/log counts
cannot prove a carried magazine was visibly rendered in the previous session.

L1_PASS:19 CPU/ABI +3 OWN D3D9 resource/GPU fixtures, plus3 MAIN compatibility
fixtures (ammo-layout, scripts-ready, holster-marker): all25 executables x86
PE32 checked.46 Python tests PASS. Includes2916 full-chain roundtrips using the
original SVCA, controller yaw/pitch/roll,3 world scales and3 gun scales, native
component/parent motion, exact physical-left motion, render/input socket
agreement, transaction resets, failed output preservation, stale socket refusal
and the complete Circle/open/dump/shoulder/insert/flick/ONE permit sequence.
The old model-only hold is explicitly exercised and FAILS the moving-component
invariant in those fixtures. Existing legacy sequence/lifecycle/native ABI,
original animation/IB, shared-depth spare/material/Reset and original-shell GPU
regressions PASS. The GPU fixtures are OWN synthetic scene/material (shells use
original extracted geometry/albedo), NOT native game lighting or drift proof.

L2_PASS: clean source0742736, x86 PE32 `/O2`, performance instrumentation OFF,
DLL3190784 bytes, SHA-256
`867644F4980EFECE47D56084C6EDD47784CFE48C22A18936E5A6AF03845426F3`.
Build receipt SHA-256
`1550120007AE20784DF1440F91ECCA96C054429F6BDEC32E650AEE166537DE4F`.
Local/private candidate (DLL/build receipt/manifest only):
`C:/Users/Trilex/vr-mods/SingularityVR-centurion-visual-feedback/build/centurion-reload/tracked-candidate-0742736/`.
Both hashes and clean source identity checked independently. Logs:
`build/centurion-reload/tracked-tests-final.log`, `tracked-build.log`; final
source-wiring tests were also rerun after the last input validation change.
Unrelated pre-existing static analysis warnings remain, not warning-free.
Subsequent documentation-only commits do not change this binary provenance.
Installed/accepted/restore identities are NOT established by this SIDE receipt;
the read-only last-session log and historical MAIN deployment record are separate
evidence. ZERO launches/deployment/restore/INI writes; L3/L4 UNKNOWN.

MAIN physical route (one continuous test, NO screenshots or mid-headset chat):
Press Circle with a partially or completely spent Centurion and check immediate
LEFT opening; rotate/tilt the gun to dump cases and verify the gun stays in the
right glove. Squeeze and HOLD physical-left grip behind the LEFT shoulder,
bring the visible ammunition to the open cylinder, release, wait2seconds (must
stay open), then flick close and check ONE refill/reserve debit without a second
gun sweep. Repeat once, pause/Options/resume, close normally and report cylinder
side/travel, carried content/fit, both-eye occlusion, gun/glove alignment and FPS;
MAIN checks new `tracked cylinder` rows (socketFresh/insertDist), native ACK,
verified deployment/backup/lease/restore and unchanged accepted systems.
