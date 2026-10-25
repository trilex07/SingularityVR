# L2 support pose and Square TMD beam — controlled candidate

The 2026-10-25 PSVR2 retest confirmed the carried magazine is stable. The two
remaining symptoms were unchanged. The archived session is in
`build/main-seeker-evidence/session-20261025-190755b/view_matrix.log`.

## First violated contracts

- L2 then left Grip **did** latch support (`support-acquire ... gate=latched`,
  frame 1363). The gun was submitted for both eyes and ridden by the right
  controller, but its original Volk body-root GPU skin changed from the normal
  `HANDS_WEAPON` pose to the native `HANDS_TMD` pose. The measured roots in
  frames 1150 and 1364 differ by about 90 UE units and a large rotation. This
  displaces the weapon before the controller ride; changing the support zone
  or aiming vector cannot correct it.
- On Square, `RvTMDPower_Age::CreateFX` produced the beam at the audited
  `RvTMD+530h` field. Ten consecutive census reads saw that same beam and a
  live owner component. The existing installer rejected the beam because its
  transient UObject Outer chain did not end at `PersistentLevel`. Its exact
  owner pointer, `RvCurvedBeamComponent` class, original vtable fingerprints,
  caller, thread and native function contract are still required.

## Changes

- `[Input] DualWeaponTmdGunPose=1` (default OFF) captures the exact body-root
  palette of the equipped Volk or Valkyrie after a successfully submitted
  normal weapon draw. During `HANDS_TMD` it applies the measured rigid root
  difference once to that same body palette. Child fire/reload animation stays
  relative to the root. The original palette is restored after stereo draw;
  failed reads/writes/ref identity use the native palette. No inventory,
  controller or support-grip offsets change.
- `[Render] TmdTrackedBeam=1` (existing default-OFF flag) now recognizes the
  transient beam through the exact live TMD ownership field and class instead
  of the unrelated `PersistentLevel` Outer rule. The original four-argument
  component update is forwarded exactly once, with its existing runtime
  fingerprint and final rendered-wrist freshness gates.
- The confirmed carried-magazine path remains unchanged.

## Verification and physical gate

`test-dual-weapon-tmd.ps1 -SkipOled`, `test-tmd-socket.ps1`,
`test-dual-tmd-gun-scale.ps1`, x86 PE32 `/O2`, and the native/compiled TMD beam
ABI audit pass. The runtime-regression CPU suite reached its D3D9 HAL fixture;
`CreateDevice(HAL)` is unavailable in this desktop test context, so that
fixture is not a rendering PASS. Steam/OpenXR/PSVR2 acceptance is pending.

In one PSVR2 session: while holding Volk or Valkyrie, use L2 then left Grip;
release, use R2 then left Grip for comparison; then use Square while looking
at the left glove. The gun should stay in the supported right-hand position
after L2, and the native Square beam should begin at the glove. The candidate
logs bounded success/refusal receipts for both paths; close the game normally
so MAIN can inspect them and verify restoration.
