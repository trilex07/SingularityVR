# Centurion second physical feedback — 2026-10-17

## UPDATE: latest physical rejection — native/export-space mismatch

The previous 0742736 candidate is **REJECTED for cylinder/case visibility**,
not physically accepted. Its synthetic chain tests used matching source
conventions on both sides and therefore missed the actual native vertex space.
Do not redeploy its historical binary as a corrected build.

Read-only last-session snapshot: header12:29:07, ending12:30:29,
`build/centurion-reload/feedback-20261017/session-1230.log`, SHA-256
`119CC80DC8986EF30978B6B3956609C14B7CA20D41E3AC35A4E83588D88871A7`.
TrackedCylinder=1; Circle/open/dump and shoulder grabs ran. Socket at dump
(-1.356,-1.340,1.238), hand(-.416,-.154,-.587), distance2.371m. Later
insertion distances2.5–3.06m. Three cosmetic shell bursts and growing native
spare draw counts do NOT establish visible content. User sees neither.

### Mechanism proved offline

The exact original cooked 5799-vertex GPU stream stores position(x,-y,z)
relative to the ActorX PSK export. All5799 positions, all influence slots and
quantized weights, and all6767 triangle windings agree under that mapping.
1067 rigid clip GPU wedges (seams included) are mapped to palette slot0.
The old SVCA cache instead contains reflected/export-space binds. Applying its
inverse bind to native vertices misplaces the clip joint by226.221775UU;
native joint Y=-113.110887 vs exported Y=+113.110887. This is independent
evidence from the actual original bytes, not an inference from a draw count.

Inputs (private, not redistributed):

- PSK SHA256 `4E18A25D5241DE1B3F18100E3BD9373D464EE16BAF3E29D7C60289AE18FE73D4`.
- Decompressed original package SHA256
  `0534C5C9A5878D280294168BD12711B99CB3948503D4ED53608A0316479D1B18`.
- Exact VB/IB fixture SHA256
  `8D380463597586DE4D55DBF176B2396AE39316D7E54B5991214F272A9EC0FA80`.

Separately, an existing archived actual skinned-arm colour shader was decoded
offline with `tools/reload/inspect_reload_shader.py`; SHA256
`C27DA11EE20D69FADAE58D3C828EA8CF688BAE80762391301E9978DA5C446330`.
It uses three skinning dot products then row-vector LocalToWorld and projection.
This supports the existing matrix convention; it is NOT a current Centurion
shader capture. No speculative transposition or register change is justified.

### Controlled correction and falsifier

Under the existing default-OFF TrackedCylinder flag only, build a native-space
copy of the original SVCA once at initialization: S*bind*S, S*inverse*S with
S=diag(1,-1,1,1). PskLocal already undoes ActorX reflection; native-space Sample
must not reflect it a second time. Both attached clip/socket and carried clip
use this SAME native cache. Original files, materials/UVs, vertices, live bone
constants, radii, original OFF route and ammo/one-permit authority are unchanged.
The shell launch already uses the fresh rendered socket; correcting that socket
corrects its spawn origin without a new cosmetic hand offset.

Falsifier: exact native GPU vertices still map far from the joint, the old
cache produces the same expected pixels, corrected carried/attached clip fails
the two-eye pixel oracle, or a MAIN session still places its fresh socket metres
from the physical gun. In that last case investigate the current gun shader/
chain with bounded capture; do not inflate radii or call successful DIP proof.

`audit_native_cylinder.py` writes only a NEW directory under this worktree's
ignored lab. `test-centurion-native-space-gpu.cpp` reads the exact private
VB/IB, uses a synthetic shader implementing the observed skin→local→projection
chain, and compares old/native mapping. It is an OWN D3D9Ex process, not Steam,
not the proxy and not evidence of real-game lighting or physical acceptance.
Final build/test receipt follows below when complete.

Offline result: exact cooked geometry with the old cache gives0/0 pixels;
native-space carried AND attached clip give12974/12975 pixels in the two
fixture eye viewports, with expected bounds/disparity. All1067 rigid GPU clip
wedges are within10UU of the corrected joint and >220UU from the old one.
The corrected attached socket feeds the actual shell Launch/Sample policy;
all3 spent cases start within5.1mm of that joint after full world/tracking
conversion, rather than metres away. Existing original-shell own-GPU fixture
still shows110/114 textured pixels and passes Reset/state restoration.
26 OWN CPU/ABI/resource/GPU programs (including3 MAIN compatibility fixtures)
and50 Python tests PASS; this remains L1/L2-side evidence, not installed L3/L4.

MAIN next test is **visibility first**, not another blind insertion exercise:
integrate just the new source delta into CURRENT MAIN, preserve its newer VFX,
HUD, hands, knife, Polish files and baseline restore hashes; rebuild/verify
candidate and use the approved deployment lease. Original SVCA/shell assets
and INI remain unchanged; existing TrackedCylinder/VisualFollowup flags suffice.
Check `centurion native space: cache=READY`, then one continuous headset route:
Circle opens LEFT beside the gun → tilt and SEE cases → take left-shoulder
ammo and SEE it in the left hand → only if visible, insert/release and flick.
Stop that route if either object is still invisible; no mid-headset chat.
After normal exit MAIN checks fresh socket proximity, ONE ACK/refill, both-eye
appearance and unchanged accepted systems. New candidate is not a restore target
or physically accepted build merely because this offline oracle passed.

### Final candidate receipt

Source `5d5afd7798dcdc40d2ed291573edcc37c4ee1567`, own branch
`fix/centurion-visual-feedback`, draft PR48. L1_PASS26 OWN CPU/ABI/resource/GPU
programs (3 MAIN compatibility programs separately),50 Python tests;
the full23-program runner was repeated on the clean final source with the
new exact native fixture. Native Sample conjugacy passes1042 original poses,
cache conversion is idempotent and refuses invalid data without publishing it.
L2_PASS x86 PE32 `/O2`, no performance instrumentation; independent PE/hash
checks agree with the build receipt. Existing unrelated compiler/static-analysis
warnings remain, so this is NOT a warning-free build claim.

- DLL3190784bytes SHA256
  `716B453BEB963FD9B666829DAFBD2C723F55A71F476543ECF242EB42BC461210`.
- Build receipt SHA256
  `EE3CA1636611EF6CF52D5EDA79639895A2C9886F244275FF0AC2D194C00405A0`.
- Private candidate:
  `C:/Users/Trilex/vr-mods/SingularityVR-centurion-visual-feedback/build/centurion-reload/native-space-candidate-5d5afd7/`.
- Full rerun: `build/centurion-reload/native-space-tests-final.log`;
  build: `build/centurion-reload/native-space-build.log`;
  exact vertex audit: `build/centurion-reload/native-space-audit-final/`.

Reproduce the native fixture with explicit original offline inputs:
`python tools/reload/audit_native_cylinder.py --psk <original-psk> --cooked <original-decompressed-package> --output build/centurion-reload/<NEW-directory>`.
Run `tools/reload/test-centurion.ps1` with existing `-NativeEvidence`,
`-AnimationCache`, `-ShellAssets` and NEW `-NativeVertices <audit>/native.svnv`.
No derived game geometry/bytecode or install assets are committed to Git.
Documentation-only receipt commits after5d5afd7 do not alter binary provenance.

MAIN should integrate the scoped production delta in animation header/d3d9.cpp
(shell header only clarifies its comment), plus tests/tools as useful, WITHOUT
overwriting its STATUS/current VFX work. Existing flags/assets are sufficient;
`centurion native space: cache=READY` identifies the new startup route.
The same exported/native-space risk should be audited on the THREE OTHER
weapons' actual cooked bytes before claiming their carried magazines correct;
this Centurion-specific proof does not validate their binds or change PR45.
Installed/accepted/restore identities remain MAIN-owned, not these hashes.
New L3/L4 UNKNOWN. ZERO Steam launch/deploy/restore/installed-file writes.

### Previous candidate history (superseded)

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
