# Kasimov optical accuracy — controlled correction

Observed: physical scope remains off-target. The last recorded outgoing native
Kasimov rays differed from the measured barrel by approximately 0.8–7.8 degrees;
the scope camera and barrel receipts agreed geometrically. This does not prove
pixel/hit agreement.

Offline RE identified all three native spread getters on the actual sniper
vtable. Each asks Pawn.IsAiming before selecting hip versus ironsight spread.
Our optical RTT intentionally leaves native ADS disabled (no full-screen zoom).
Therefore the native spread policy can remain hip-fire while using the optic.

`Input/SniperOpticalAccuracy=1` (default OFF) answers those three exact native
queries as aimed, only for the local pawn with a current near-eye Kasimov optical
frame. Whole-method fingerprints and reflected field contracts must pass first.
All other IsAiming callers retain their native result. The existing hook is reused;
no arbitrary spread overwrite, zero-spread cheat, camera ADS or additional capture
is introduced. The getters are shared native code: caller identity plus active
local optical state is the boundary, not a claim of direct weapon-self identity
inside Pawn.IsAiming.

Original sniper package hip min/max are 1.0; aimed min/max are inherited and must
be read at runtime, not assumed zero. Native crouch/movement/stat modifiers and
spread settling remain intact. Lowering the optic immediately removes spoofing.
Accuracy-only operation does not invoke native slow-time stop/toggle.

Offline audit: `tools/fx/audit_scope_ballistics.py --exe <owned executable>
--text <matching archived .text> --report <private output>` checks full body hashes
and exact IsAiming call/return locations, without publishing disassembly.
CPU tests cover all allowed callers, disabled/non-optical/foreign-pawn refusal.

Falsifier: native aimed queries confirmed but stationary supported shots still
miss the optical centre beyond native aimed spread. Then investigate ray/camera
transform and lens parallax, not another spread constant. Physical validation
must compare stationary near/far shots and lowered-optic behaviour; L1/L2 alone
does not establish corrected alignment. Steam remains untouched during preparation.

## Physical result 2026-10-20, A048D4C2

FAILED alignment: user still reports off-target shots. Runtime proves all three
aimed queries were accepted; reflected aimed min/max are 0/0 (hip 1/1).
Outgoing rays still differ by ~0.7–0.9 degrees. Replaying the existing pre/post
collision receipts with the logged trace signs places most residual in native
pitch BEFORE controller rotation (~0.63–0.93 degrees for eight recorded shots).
Rounded historical endpoints limit precision; this does not identify recoil as
the producer. Baseline DLL/INI restored and independently rehashed.

Next correction observes the exact native AddSpread input FRotator (central aim)
and output FRotator (spread ray), rather than assuming the input equals cached
HMD yaw/pitch. Its whole 536-byte body proves hidden output pointer + three ints,
ret 0x10. For the same local weapon, thread, fire sequence, fresh optical frame
and matching output ray, collision direction is rigidly rotated from that actual
native central axis to the measured barrel. The native spread cone and range
remain unchanged. Every mismatch falls back to the previous path. No guessed
angular bias or spread-total write is used.

Up to eight paired shot receipts also save the completed capture's exact native
view/projection matrices and native spread values. They distinguish a remaining
capture projection/parallax error from the pre-spread aim mismatch. No extra RTT
readback, per-frame property walks or extra scope render is introduced.
