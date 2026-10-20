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
