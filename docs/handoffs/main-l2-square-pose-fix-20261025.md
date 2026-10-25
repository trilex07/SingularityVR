# L2 weapon pose and Square TMD attachment, 2026-10-25

## Physical result that prompted this change

The previous PSVR2 candidate failed: after L2 the right weapon moved away from
the right hand; Square's Age effect remained beside the left TMD. The accepted
Steam installation was restored and hash checked before this work.

## Evidence and correction

- The archived trace at `build/main-seeker-evidence/session-20261025-193836-5426/view_matrix.log`
  records Volk normal frame 2459 and L2 frame 2461. The old correction compared
  un-ridden gun roots. With `BuildGunC` included, the L2 gun was about 59 UU
  from the position implied by the visible right wrist; the original uncorrected
  source pose differed by only about 11 UU. The normal and L2 final right wrists
  differed by less than 1 UU. The new code records the *final* normal gun to
  visible-wrist relationship, advances the last completed stereo wrist by the
  raw right-controller delta, and transforms the entire gun skin palette once.
  It refuses stale, foreign, invalid and non-stereo witnesses.
- Native `RvTMDPower_Age::CreateFX` attaches an `RvCurvedBeamComponent` through
  the skeletal socket. `SkeletalMeshComponent::UpdateChildren` composes the
  socket, bone and child's relative transform before calling the child update.
  The earlier hook replaced that complete parent with `SocketLocal * trackedWrist`
  and therefore discarded the child's native offset. The new hook applies
  `nativeParent * inverse(nativeWrist) * trackedWrist`. The original component,
  target, curve and power logic remain authoritative.

## Verification and physical acceptance

- `tools/fx/test-tmd-socket.ps1`: PASS, including the real callback ABI and four
  wrist rotations with different source and tracked wrist matrices.
- `tools/tests/test-dual-weapon-tmd.ps1`: PASS, including the archived frame
  matrix regression, weapon child pose, stereo equality and critical gameplay,
  WeaponLocker and OLED regressions.
- x86 PE32 `/O2` build: see `spikes/view_matrix/d3d9.build.json` for the exact
  hash after the final build. These are offline results, not PSVR2 acceptance.

One short PSVR2 session remains: hold Valkyrie or Volk in the right hand, press
L2, then take the front grip with the left hand; compare with R2 then grip.
Release grip and use Square with the left TMD near a target. The gun must stay
with the right hand and Square's effect must begin at the visible TMD glove.
Check a magazine pickup only if convenient; the previous candidate already
passed its physical flicker check.

## Follow-up after physical E73087DC (same day)

The user saw the black weapon-shaped region again and could only acquire the
ordinary two-hand grip awkwardly. Square was not observed. The log confirms
`TrackedDepthPrimeFix=0`, `TrackedDepthSourcePose=0`, and
`SupportGripNearZone=0` in this candidate, whereas the prior measured candidate
had all three at 1. This INI regression restores a previously documented
foreground depth producer and the stricter ordinary grip cone; it is not
evidence that the TMD Age beam failed.

The E73087DC trace also proves a separate gun-pose contract violation. At frame
35921 the Volk body is corrected once, then submitted twice with its original
palette after the TMD wrist publishes in the *same* frame. The old reference
overwrote the normal gun/wrist anchor with the differently animated TMD wrist,
and `frame<=wristFrame` refused subsequent submissions. The 50 ms witness cap
also refused draws at 62 ms in the same frame. The correction now preserves the
paired HANDS_WEAPON wrist/controller anchor; a stereo TMD wrist refreshes only
tracking freshness. Same-frame and next-frame gun draws use the same predicted
pose, subject to exact pawn/weapon/mesh, tracking and two-frame/500 ms gates.
No native gun palette, inventory, firing, TMD power or right-hand input is
written outside the existing default-OFF render path.

The older near-zone PSVR2 log records valid Volk Grip presses at 10.15 and
11.94 cm refused solely by the 12 cm minimum. The opt-in ordinary-long-gun
near-zone now accepts down to 4.5 cm, matching its existing near-hand maximum
of 30 cm. Seeker, pistol, reload ownership and the ordinary OFF route retain
their previous policies. The bounded support-acquire receipts remain available.

Offline: dual-weapon/TMD suite and native TMD socket tests PASS; x86 PE32 `/O2`
build PASS. The D3D9 and D3D9Ex depth-prime pixel test PASS with hardware GPU
access; its first isolated run could not create a HAL device (8876086A).
Physical acceptance of the combined follow-up remains PENDING. The next
candidate must enable the three previously missing INI flags together with
`DualWeaponTmdGunPose=1`, `TmdTrackedMuzzle=1`, and `TmdTrackedBeam=1`, while
preserving every accepted INI entry. In PSVR2, first check the black region on
Volk/Valkyrie over a floor; then L2 -> left Grip, R2 -> left Grip, and Square
at a visible TMD target. Close the game normally; MAIN reads the bounded logs
and verifies restoration hashes.
