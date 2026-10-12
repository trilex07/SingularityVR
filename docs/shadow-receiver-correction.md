# Shadow receiver experiment, 2026-10-12

Status: BROKEN experiment, physically rejected. No fix claim.

## Physical falsification of 88411b4 / ED15225F...

PID24340 executed ~180 corrections and ~180 safe rejections/second, zero
restore failures. Complete trace: 1235 calls / 276 groups / zero overflow.
Both eyes' first receiver draws contain new inverse matrices. User reports
strange rendering and shadows apparently moving with the right VR controller.
Correction was immediately disabled by removing the owned flag, leaving HQ
Decals ON; log confirms enabled=0. This rejects using every receiver volume's
current VS transform as the scene-depth camera. Captured first/last draw numbers
982/1206 and 983/1207 show two same-state draws per eye; first-only groups hide
their different constant semantics. Existing StereoPairBody can compose
BuildGunC/BuildTmdC when g_thisDrawMoved is nonzero, while those matrices need
not describe the scene alpha-depth producer. This is a hypothesis, not yet proof
that that branch ran for the offending receiver.

After disabling the correction the user additionally reports two screens rather
than stereo, weapon not following hand, and no observed shadows; game closed.
Final log contains c0(stale), zero split draws in some windows, and xrSyncActions
failures with hand position invalid. It also contains pause/menu transitions.
These establish a broader failed runtime state, not its cause: loss of XR focus
or scene mode/lifecycle must not be silently blamed on the user or assumed to
explain the entire regression. Controlled cold start on exact accepted DLL is
the next safety gate before deploying the expanded read-only diagnostic.
After exit all three accepted hashes verified, experimental flag absent.
The CONTROL request did not produce a second trace before shutdown; no OFF
capture comparison is claimed.

## Accepted baseline control, PID31176

User physically confirms accepted 9FBA308C cold start restores stereo and weapon
controller tracking. With HQ Decals ON, square shadows remain, distant shadows
move with the gun, and the tracked gun sometimes turns black while its texture
appears frozen elsewhere. User explicitly says the latter also happened before
and had not been reported. Therefore these gun/light artifacts are baseline bugs,
not evidence unique to the rejected receiver correction. No causal conclusion
about the temporary stereo loss follows from this control alone.

Game has exited. Next candidate cb9902e / EA298F2F... is read-only context
diagnostics with the correction flag absent. Two same-head-view captures with
different right-controller poses test whether the receiver's first/last VS/PS
matrices or moved context follow the controller. Maintain HQ Decals ON throughout.
Trace request now waits for 15 consecutive focused gameplay frames with both
hand orientations/positions valid, and rechecks focus/positions before arming.
This scopes diagnostics only; it does not change game input or rendering policy.

## Context A/B, PID43920

User confirms the distant shadow or black/frozen gun changes with right-hand
movement. A/B complete: 1073/1049 calls, 288/252 groups, zero overflow; focused
session=5 and both positions valid. Receiver moved=0 in both eyes and both
captures, so BuildGunC inside this receiver's StereoPair call is NOT established
as the direct cause. Camera orientation changed substantially between captures;
do not attribute all matrix differences solely to hand movement.

First/last receiver constants differ despite the same shader/resource identity:
MinZ_MaxZRatio c2 near coefficients ~9.99/~0.999; ScreenToWorld's unused W
component ~1/~10. This explains the earlier strict affine guard rejecting half
the calls, not the full artifact. PS never consumes that W component.

Actual source path found: receiver scene sampler is HDR113 texture, copied from
another HDR113 texture by PS 0df8d996... (TheTexture), whose producers include
PS c965e1e958d3de23daa235d80271d96cb693201e07d4a705986c5c2de8e4876b.
Private disassembly shows producer VS passes clip W to texcoord5.w and producer
PS writes alpha = MinZ_MaxZRatio.x / clipW + MinZ_MaxZRatio.y. Thus native alpha
really encodes depth, and consumer inverse coefficients can be checked against
the producing pass, rather than guessed from hardware Z.

Remaining hole: bound RT alone does not prove alpha writes. Some intervening
box-volume draws use a constant-output PS, potentially stencil-only. Expanded
read-only candidate records color-write, depth, stencil, RGB/alpha blend states
in the grouping key (13 state reads/final call, only one requested bounded frame).
This distinguishes actual depth-alpha producers from non-writing volumes and
captures different receiver coverage/state instead of silently grouping them.
No new matrix correction until producer/depth/coverage semantics are verified.
After A/B exit accepted DLL/INI/UI hashes were all restored and verified.

Next controlled candidate is read-only: correction flag remains OFF, trace key
includes moved context, repeated 8/12 volume draws retain last VS c0..3 and PS
c0..14 (<=128 extra reads/frame). Compare first/last matrices and native world
coordinate conventions before constructing another correction. No hand or
weapon transform changes and no general stereo changes.

Base: main 9e92a4f, accepted menu DLL 9FBA308C...99C4500. Frozen
UI/weapon/hand/texture/stereo paths are not changed.

Same-session physical ON/OFF/ON traces (PID 9576) contain 1101/909/1041
final calls, all without overflow. User reports HQ Decals OFF also removes
ordinary shadows, so OFF is NOT an acceptable fix or decal-only proof.

The native receiver PS SHA256 28e0113352db6dff68315a029a7791467fe928d56d79ab7fc13776981af5a044
and VS d09ad82cc046f97724dcffcbb7976c271df034414c08742fab66810e94038f19
occur in both ON traces, not OFF. VS copies per-eye clip position to its PS
varying. PS derives [clipX,clipY,linearW,1] from scene alpha depth, then uses
ScreenToWorld c3..6 and ScreenToShadow c7..10. These inverse matrices are
identical in both eyes while VS ViewProjectionMatrix c0..3 differs.
Actual 8-vertex / 12-triangle volume draws sample scene HDR and 512-square R32F
shadow depth. This identifies a real shadow receiver, not yet the visually
confirmed square producer. Proprietary bytecode/traces remain private.

Hypothesis: reconstruct with inverse of the actual eye's forward projection
columns X,Y,W,affine1. Preserve world-to-light transform by composing the
old world inverse and old shadow transform, rather than changing shadow-map
generation or disabling a pass. Captured left-eye reconstruction test reduces
error 522.507 -> 0.000061 units; synthetic 54-point two-eye invariants pass.
This is algebraic evidence only: linear-depth semantics and visible outcome
still require runtime/physical falsification.

Experimental flag: shadow-receiver-fix.flag beside view_matrix.log. Missing
means OFF. File expires after 20 minutes; one-second polling supports same-
session A/B with HQ Decals left ON. Bounded bind-time bytecode classification
uses exact length/FNV64 identities associated with the SHA256s above, retains
at most 1024 COM shader references until reset, and never queries bytecode in
draws. Only the matched shader pair, scene target and 8/12 volume is changed.
The leaf call temporarily uploads c3..10 and restores them on every HRESULT
path. Restore failure latches the correction OFF and is logged. Aggregate
execution/rejection/restore counters are logged once per second.

Falsification: applied must be nonzero, rejected/restoreFailures zero; compare
the same scene with correction OFF/ON while HQ Decals stays ON. Ordinary
shadows must remain, squares must improve in each eye, hands and accepted menu
must remain unchanged. If no change, reject this receiver as sole producer and
use captured output to locate the next contributing pass; do not expand broad
register heuristics. After test restore accepted DLL and verify all three hashes.
