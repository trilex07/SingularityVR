# SOL: follow-up to bc3ab34 controlled PSVR2 evidence

Branch `fix/sol-bc3ab34-runtime-evidence`, base `bc3ab34`. Source evidence:
`handoff/bc3ab34-psvr2-controlled-evidence`, including three archived logs and
`deployment/DEPLOYMENT_HOLD.md`. The tested bc3ab34 DLL failed/partially passed;
Steam was restored by MAIN to accepted `514bb03`. This branch did not access
Steam or launch the game. **No physical PASS is claimed.**

## Independent commits and decisions

### P0 — gun scale, `d92a4e5`

**Proven source defect:** `PropOffsetInChain` resolves fields through
`WantedIdx`, but `g_wantNames` did not register `mGunScaleCtrl`, `ControlName`,
or `BoneScale`. Therefore the reflected control lookup refused before the
hook could arm. The three names are now registered. A bounded event-driven
witness logs L2 press/hold/release and R2 with exact camera/controller/cached
pawn IDs, selected class, property offsets, inview mesh, and armed state.
Existing fingerprint, caller, class, control and ownership checks remain.

**Not yet proven:** gameplay ownership may still be transient or the actual
control may belong to a different object. A MAIN test must show
`dual-tmd gun-scale: ready reflected` followed by
`local gun_scale scale=0 -> 1` during held L2. A new specific witness refusal
instead is actionable; a missing scale receipt is not success. If the override
executes but the gun remains invisible, inspect all five weapon bone matrices,
not just c5. No speculative scale write was added.

L1: registry regression and x86 MinHook 10,000-call contract test PASS.

### P1 — right glove, `b4fb5ef`

The archived single CPU receipt did not establish final GPU submission or
continuous tracking. The existing `tmd-assembly bound` receipt sampled left
wrist slot 1, not the right wrist slot 19. Added bounded final-state witness
for material-0 right-wrist c62–c64 and LocalToWorld, stratified by hands state,
L2/R2 and both eyes. It records actual draw route and draw-segment count.

**No geometry change:** without held-L2 final draw evidence, the first
wrong transform is unproven. Falsifier: if c62–c64 tracks the right controller
in both eyes through L2, the problem is downstream mesh/vertex ownership;
if it remains near the body or the material-0 route is absent, the witness
identifies the earlier failed gate. Do not treat one receipt as sustained pose.

### P2 — floating knife, `20f293b`

The archived route was `jointPose=0 glovePose=0` with TMD and right-hand knife;
the noTMD-only native joint could not mount to the active TMD glove. The TMD
material-0 path now publishes its **final rendered** right-wrist skin and
LocalToWorld when the native cuff route is usable. The accepted SVNG mount is
retargeted by `mount * inverse(noTmdBind) * tmdBind`; this preserves its
physical wrist relation across the two original bind spaces, without a guessed
offset. The shared-depth knife draw occurs only after a successful glove draw.
Missing bind/pose/cuff/IB retains the old fail-closed fallback. No forearm is
restored and the noTMD knife path is unchanged.

L1: knife suite PASS, including four wrist rotations and retarget identity.
Physical falsifier: if the knife still floats, check the actual TMD glove
draw receipt, its final wrist skin, and whether `DrawKnifeWithNativeGlove`
ran in both eyes. No physical alignment claim yet.

### P3 — Volk carried magazine, `dddb35f`

The archived test proves TMD left-wrist publication, stage 12, stereo replay
and native ammo transfer. The original private-PSK fit has the **same** Volk
mount for noTMD and TMD gloves; this does not justify an arbitrary offset.
Added default-OFF `[Diagnostic] VolkCarryPoseProbe=1`: eight samples at least
400 ms apart, recording same-transaction final glove wrist, carried-magazine
matrix, controller hand, wrist age and source (`1=noTMD`, `2=TMD`). The existing
native replay counter supplies draw confirmation. Reload FSM, wrist conversion,
mount, texture and render state are unchanged.

If `deltaMagHand` is consistent but wrong while wrist tracks the controller,
compare the original glove palm surface in the measured local wrist basis.
If the wrist itself is wrong, correct that upstream transform; do not tune the
magazine mount. If matrices are correct but the picture is offset, inspect
clip bone palette/vertex ownership and replay, not the input FSM.

L1: `test-weapon-carry-ray` x86 `/O2` PASS, 4,000 translated rays,
source/transaction/age and wrist rotation checks; carry wiring tests PASS.

## Build and broader regression

Final offline build: x86 PE32, machine `0x014C`, optional header `0x010B`,
`/O2`. DLL SHA256 `9B22770F44C3931EAD88332D61B18843128972110B7AA2317E385FB5619D6D69`.
Built against offline `research/native-hands-20261007`, not Steam. Build
warnings remain; no compile/link error. Dual-weapon/TMD, short TMD, knife,
WeaponLocker, OLED and relevant reload tests passed. These are L1/L2 only.

## Smallest justified MAIN validation

1. Preserve the accepted `514bb03` DLL/INI hashes, deploy this branch only
   as a controlled candidate with existing feature gates and
   `VolkCarryPoseProbe=1`. Do not merge before physical acceptance.
2. In one PSVR2 session: with Volk and TMD, hold/release L2, fire R2 while
   L2 is held, then observe the right weapon/glove. This tests P0 and gathers
   P1 final-eye receipts. Hold the knife in the right hand and look at the
   TMD glove to test P2. Carry one Volk magazine to collect P3 matrices.
3. Close normally and archive the full log before further launch. Check the
   exact receipts and user-visible results independently. Restore verified
   accepted files on failure. If only P0 needs checking, a shorter L2/R2
   session is sufficient; do not require another full A/B/C walk-through.

The first runtime gate is P0 readiness. No statement here elevates an offline
build or matrix identity test to PSVR2 visual proof.
