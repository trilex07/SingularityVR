# L2 and Square: native audit after the corrected PSVR2 feedback

## Physical evidence and scope

The user's corrected report is: Square's effect still appears beside the TMD;
the first L2 use leaves the gun stable, but later L2/head motion couples the gun
to the head. There is no Seeker in this save. The prior response that all three
were correct was accidental. No new Steam test is claimed by this audit.

Accepted Steam DLL remains SHA256
`610F4C21C4E610C40CDF5381B8D8E98675482E409669329C3EDEF23CCD8AEFF1`.
The diagnostic run's installed files were restored and independently hashed.

## L2: first violated contract and minimal correction

The last diagnostic log records a Volk gun draw at frame 1293 refusing
`CorrectGate::Frame`, with the same pawn/weapon/root, a 47 ms old wrist and
`wristFrame=1290`. The only failing condition is the 3-frame gap against the
2-frame maximum. The gun forwards its original palette on that draw.

`Reference::ObserveWrist` also required **the new, two-eye TMD wrist** to be
within two frames / 500 ms of the *previous* wrist. Once a draw gap exceeded
either bound, later valid TMD witnesses could never advance `wristFrame` until
a new normal `HANDS_WEAPON` pairing reset the reference. This is a stale-state
latch. It explains the time-dependent regression without changing weapon aim,
camera or glove geometry.

The correction accepts a new, validated, same-key, monotonically newer
TMD wrist after a gap. `Correct` still refuses gun draws until that witness
arrives, and still enforces the current witness's two-frame / 500 ms freshness.
It retains the original normal-hand gun/wrist anchor and raw-controller delta.
The `/O2` CPU regression now reproduces a missed three-frame gap, verifies
refusal before the next wrist, then verifies recovery at the following draw;
rewind, time reversal, different weapon and invalid bone count still fail.
This is **L1/L2 evidence**, not physical acceptance.

## Square: source verified, visible producer still open

The native `RvTMDPower_Age::TickFX` at `0xFE4570` reads the owned curved
beam's world start at `component+0xA0`, interpolates the native target, and
calls beam virtual slots `0x284` and `0x27C`. Slot `0x27C` at `0xE62740`
inverts the beam's LocalToWorld at `+0x70`, converts the world endpoint to
local space and passes it to material instances. The diagnostic run shows
the post-update beam start equal to the corrected tracked parent and within
about 0.06-0.17 UU of the published TMD socket in the sampled steady frames.
Moving the beam origin again is not supported by these measurements.

The *only* `TMD tracked muzzle: native TMD_Muzzle` query receipt in this
session was at log line 5146 / frame 1665, adjacent to an **Impulse** event.
The accepted **Age** activation is at line 8689, followed by the beam parent
updates at lines 8712-8812, with no socket-query receipt for Age. This
excludes the existing `GetSocketWorldLocationAndRotation` hook as the observed
route for Age's offset particle. It does not identify the particle's final GPU
draw or prove whether that particle was visible in the user's photograph.

The original `RvGame.decompressed.xxx` contains a **second FX route** that
the beam hook does not handle. The `TMDPowers_SP.RvTMDPower_Age` instance
references two `RvEffectObject` subobjects. Both have `mBoneOrSocket =
TMD_Muzzle`; their `mParticles` reference original assets
`fx_FP_TMD_ageOld` and `fx_FP_TMD_ageNew`. The `mStartFX` property aliases
the `ageNew` object. This is not just an unused asset declaration: the original
`RvEffectObject::PlayInitialEffects` UnrealScript bytecode references its
`mParticles` field at serialized function offsets 53 and 252, the
`mbAttachToOwner` field at 477 and 836, and `mBoneOrSocket` at 533. The
native `RvTMDPower::StartFirePower` body at `0x100D790` invokes a separate
effect route after activation via its object field at `+0x6C`; the exact live
identity at that field still needs a runtime witness. Therefore the particle
route is separate from the curved-beam child, but its visible position has
not yet been measured.
The exact live particle component, its actual socket/parent transform and
GPU draw identity have **not yet been observed**. This is a strong candidate
for the offset visual, not proof that the screenshot's orange effect belongs
to this component. A broad particle or hand offset would be speculative.

Native inputs audited read-only: installed `Singularity.exe` SHA256
`BD459ACD87D4B82693928DEE4ED437C086D5928A8E0428F3AD54B6FC114C0F51`;
private original package SHA256
`09DE450D9F620952C3701D874BEB90A8BD9235263975157A641B9E3441336C92`.
The disassembly helpers and detailed output are under ignored
`build/main-seeker-evidence/`; proprietary asset bytes are not committed.
The reproducible read-only audit is `tools/tmd/audit_age_fx_routes.py`; its
private JSON result is `build/main-seeker-evidence/age-fx-routes.json`.

## Next falsifiable observation

Identify the **live** `fx_FP_TMD_ageNew` or `fx_FP_TMD_ageOld` particle
component used by Square,
its emitter/owner identity, parent socket, post-update world transform and
submitted draw in both eyes. Compare its position with the already measured
beam start and tracked `TMD_Muzzle` in the same frame. If the particle is
offset while the beam start is correct, correct only that component's native
attachment using the tracked socket. If it is already aligned, inspect the
render material/target or a different effect; do not move the glove.

The L2 patch is ready for controlled physical validation only after this
separate Square producer is resolved or instrumented. Seeker remains
**NOT TESTED** because the user has no Seeker in the current save.
