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
