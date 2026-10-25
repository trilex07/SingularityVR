# Two-hand acquisition and carried magazine, 2026-10-25

## Physical result and archived evidence

The `A2BE8D43` PSVR2 candidate removed the transparent cutout. The R3 wheel
looked correct. With Volk/Valkyrie the left Grip had to be brought below the
visible gun before two-hand support latched. The new magazine flickered **after
pickup from the left shoulder**; the magazine in the gun looked normal.

The accepted installation was restored and hash-verified. The archived session
is `build/main-seeker-evidence/session-20261025-172818/view_matrix.log`.

`SupportGripProbe` captured 24 attempts. On Volk, the controller and Grip were
valid, no knife/reload/cylinder owner blocked the input, and many misses had
`gate=forward-cone`. One measured miss was 0.1683 m away at alignment -0.2038.
A later attempt acquired only after alignment exceeded +0.5. The production
cone uses the primary controller's raw -Z direction, not a measured point on
the displayed fore-end. This proves the first refusal gate; it does not prove
the exact visual grip position.

Valkyrie logged `grab-fresh`, a TMD material-0 wrist publication, native
magazine replays, `inserted/request`, and `native-ammo-ack`. Those receipts
prove the magazine was produced and some frames drew; they do not establish
continuous pixels in both eyes. The carried copy requires a current wrist
witness with exact kind, pawn, weapon, transaction, finite matrix and <=150 ms
age. The existing log has no frame-by-frame gate result during Carrying.

## Controlled candidate

`[Input] SupportGripNearZone=1` (default OFF) admits a **new** ordinary
long-gun support grip within 0.30 m of the primary controller when alignment
exceeds -0.75. It leaves the existing outer forward cone, Seeker, pistol,
Grip-release, knife ownership and reload ownership unchanged. The measured
Volk miss is accepted by this near zone. Falsifier: the user still needs to
move below the gun, or an unintended near-body support latch appears.

`[Diagnostic] ReloadCarryContinuityProbe=1` (default OFF) writes at most 48
state-transition receipts per weapon profile. For a Carrying draw it records
preparation, native replay count, current wrist, staged wrist, source, age and
frame. If `replay=0` coincides with the visible blink, the first missing gate
can be fixed without substituting a guessed controller transform. If replay
stays continuous, the wrist hypothesis is falsified and the next investigation
must inspect per-eye draw content/depth or material state.

## Follow-up: bounded carried-wrist continuity

`[Render] ReloadCarryTrackedContinuity=1` (default OFF) addresses the known
rendering gap when the original left-glove draw does not publish a new wrist
within 150 ms while the fresh magazine is being carried. It first uses the
same final-bound wrist as before. For a gap of 151–350 ms, it continues that
**measured** wrist through the change between the recorded and current tracked
left-controller poses. It refuses a foreign kind/pawn/weapon/transaction,
untracked or stale controller, stale initial sample, displacement over 0.75 m,
and a gap beyond 350 ms. The source is never an AIM-space guessed wrist.

The prior physical log proves magazine creation, TMD material-0 wrist
publication and repeated native replays; it does not prove that wrist expiry
caused every observed blink. The continuity receipt plus bounded transition
probe distinguish remaining CPU gate loss from native D3D material/depth
flicker during the next physical test. The measured Volk acquisition miss is
accepted by `SupportGripNearZone`; that near-zone is still physically pending.

## PSVR2 protocol

From the same save, hold Valkyrie or Volk normally and press left Grip at its
visible fore-end, then release. Eject a magazine with Circle, take a fresh one
from behind the left shoulder and hold it in view for about five seconds before
inserting it. Report whether support latches naturally and whether the carried
magazine flickers; close the game normally. Do not switch from the headset to
chat during the run.

Offline tests and a successful build are not physical acceptance. Do not
promote the candidate until both support and magazine presentation pass.
