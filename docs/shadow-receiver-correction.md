# Shadow receiver experiment, 2026-10-12

Status: IMPLEMENTED_NOT_VERIFIED. No physical fix claim.

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
