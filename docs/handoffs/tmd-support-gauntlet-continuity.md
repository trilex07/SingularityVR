# TMD on the supporting wrist — isolated candidate

Branch: `fix/tmd-support-gauntlet-continuity`, base `cac4ce3`.
User changed the acceptance requirement: supporting the weapon must **keep**
the TMD and short sleeve, not hide the device. This supersedes earlier
hardware-hidden-on-Grip acceptance criteria. No Steam deployment or launch.

## Proven conflict

The archived diagnostic log SHA256 is
`5D3CA90DE512F8293276045E23EC5B6F757CB98564690DB16FF1706BC444CBC6`.
At frame 4905, hands=2/support=1, lines 7573–7582 show material 0 using the
SVNC IB `73173080`, 3994 triangles, in both eyes; material 1 has `segments=0`.
The glove is submitted, but its adjacent device/sleeve is intentionally removed.
`FgHidden` and the post-palette `dualHardware` gate both enforce that policy.
This proves the missing adjacent sleeve route, not a topology hole in SVNC.

The source SVNC left glove is closed (1786 faces). Its nearest source sleeve
vertex is 0.192703 UU away. No new cap, offset or remodel is justified.

## Correction and controls

New opt-in `[Render] TmdSupportGauntlet=1`, default 0. Requires the existing
short-gauntlet, native gloves/cuffs, hands-only mode 2, independent tracking,
right-handed dual context and both tracked controllers. Otherwise the old
support visibility policy remains. MAIN must retain all other accepted flags.

- Only the existing mixed material-1 filtered path is admitted on support.
  SVTG still submits 7110 faces per eye; original 7704 faces are never enabled
  by this change. The 49-run original hardware-only fallback is retained.
- Glove, hardware and seven staged sleeve bones use the same support target.
  The accepted `685e42dc` stage transforms ORIGINAL sleeve matrices once,
  preserving animation and avoiding mixed-palette stretching.
- No glove-only parking shift while the support assembly is enabled.
- Verified cooked socket attachments use that same support target in both
  eyes rather than remaining at the free controller target. CRC identity and
  draw classification remain unchanged; no generic FX/mesh exception.
- Existing weapon-authored or fore-end target selection is reused. No new
  weapon offsets, right-hand pose, finger pose, input or power edits.
- L2 Impulse and Square Age remain reserved during support. Releasing Grip
  while holding a power button still requires release/repress; keeping the
  device visible must not accidentally fire a power. R2 mapping is untouched.

Missing/corrupt SVTG or missing cuffs keep the old support hide policy.
GPU allocation/bind failure retains the existing hardware-only fallback, not
an unfiltered arm. It may lack the sleeve and is NOT visual acceptance.
Index and palette guards/restoration are unchanged. No render-loop allocations
or GPU readbacks were added. Performance in PSVR2 is not yet measured.

## Evidence gates

- L1: new executable tests 32 combinations of four weapons, hands=1/2,
  support/release and two rotations; verifies the common bone/socket transform,
  all 15 material-1 matrices, single sleeve transform, invalid-target fallback,
  enable/hide gates and input release-to-rearm.
- Existing dual-TMD suite: CPU input, reload, measured aim, cuff, transition,
  WeaponLocker preview/policy/layout, Python wiring, OLED CPU and D3D11 PASS.
- Original PSK/cooked/SVTG checks PASS: original index identity/material,
  five geometry rotations, 414 mixed-influence triangles, two palette rotations,
  unchanged closed SVNC topology. Private files remain outside Git.
- Short-gauntlet dispatch/failure/reset/restoration tests PASS.
- D3D9 and D3D9Ex standalone fixtures PASS: 2 x 7110 submissions and
  3304/3304 nonblack pixels; original IB restored. HAL creation was denied in
  the sandbox; the same executable passed outside it. These are synthetic
  positions, NOT game-material/PSVR2 evidence.
- x86 PE32 `/O2` build PASS. Existing analysis warnings
  are not represented as a warning-free build.

Unchanged required private SVTG SHA256:
`452EA0C0DDFB4721A7F9B9718629975CF021F9E079E1CD92235C076E9E376192`.
Runtime relative asset: `vr-assets/hud/katorga/glove-animation/tmd_short_gauntlet.svtg`.

## Separate unresolved interaction defect

The L2 weapon disappearance is NOT fixed here. Evidence branch
`handoff/volk-skin-8fbb9f4-diagnostic-evidence`, commit `9a4d492`, preserves the
full log and analysis. It is classification E for producer attribution:
all five consumed bones collapse in some incoming/submitted draws, but the
exact L2-held R2-restoration interval was lost after the 64-sample cap, and
upload caller attribution does not match the gun. Do not replace bones with
identity, force native hands state or freeze a cached palette based on this.
General TMD particle origin and physical-controller alignment are also not
certified by the shared socket-target math. No new offset is introduced.

## MAIN acceptance (after review only)

Enable only the new flag over the accepted short-gauntlet/dual baseline.
Hold Volk with Left Grip, rotate both wrists, release and reacquire twice:
the complete glove, short sleeve and TMD should remain a connected assembly
on the support hand, with no long arm, detached components or duplicate device.
Check R2, then Grip release and L2/Square rearm; treat the already known
L2 gun disappearance as a separate failure, not a new success claim.

Require post-submission receipt:
`tmd-support-gauntlet: ... support=1 eyes=2 tris=7110 indexRestore=OK paletteRestore=OK`
and a corresponding support=0 receipt on release (max eight transitions/process).
They prove draw/restoration only; physical continuity still requires PSVR2.

Recommendation: GO for code review and controlled support-appearance testing;
NO-GO for promotion as a complete TMD interaction fix.
