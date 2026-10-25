# Dual-hand action interference — bounded pose follow-up, 2026-10-25

Historical diagnostic checkpoint: the subsequent implementation and recovered
session evidence are in [the action-pose candidate](sol-dual-hand-action-pose.md).
The statements below describe the earlier diagnostic-only commit.

Scope: follow-up on `feature/sol-seeker-back-slot` / PR #60, retaining all Seeker
work. **The new action-interference fix is NOT implemented or physically proven.**
This commit adds the missing discriminator; it does not replace the accepted
baseline or change powers, firing, input bindings, rendering matrices or support.
MAIN owns deployment. No Steam access or game launch was performed.

## Current evidence and limits

User: L2/Square lowers the right weapon until R2; R2 then disrupts the left TMD.
Only nearby left Grip should couple the hands for two-handed support.

Confirmed in current source:
- Support acquisition uses raw left Grip AND the existing distance/weapon/tracking
  latch. Neither L2 nor Square activates that latch. Native power reservations
  already exclude support, reload, menus and knife. Do not remap buttons to fix a pose.
- `DualWeaponTmdGunScale` neutralizes only the native gun-scale control's hidden
  result. It does not neutralize the native hand's translation/rotation.
- `PrepareIndependentLeft` retains source animation, then applies controller
  deltas. `BuildGunC` similarly does not invert the current native right wrist.
  The accepted authored weapon grip deliberately excludes the older free-right-
  wrist translation fix (`!weaponSingleHand`). Removing that guard alone would
  affect the glove, not necessarily the gun, and could separate them.
- Archived accepted-hud-36081c3d INI has Ready/GunScale/RightWrist all enabled.
  Therefore this is not established as merely a missing flag.

Confirmed original binary evidence (archived inputs, never installed Steam):
- EXE SHA256 `BD459ACD87D4B82693928DEE4ED437C086D5928A8E0428F3AD54B6FC114C0F51`.
- Text SHA256 `858F2FC33B1C99B3A4A27A817A11205DA66414EE1B030A03DF821F1AD6A0C78A`.
- Native SwitchToTMD at `0xFD17F0` plays animation through pawn vtable +0x6B8,
  sets control +0x1024 strength to 1 and +0x1028/+0x102C to 0.
- SwitchToWeapon at `0xFD1960` stops/plays animations and reverses those strengths.
  These functions also perform native gameplay work: suppressing either whole
  function is not a justified presentation-only correction.
- Reproduce privately with existing `tools/reload/inspect_impulse_pose.py`, using
  the archived EXE/text, `--address 0xfd17f0 --address 0xfd1960 --size 480`.

Confirmed original skeleton/animation evidence:
- TMD PSK SHA256 `F64F5EE7ABD6A2DFDA0A686B3FF16C4EAD13325D4CEB352D987C8895EF269A5B`.
- TMD PSA SHA256 `CD9B6B782505D3053591C84C5543FCB74CB572A33CE71B6857DFD6909015DF56`.
- `tmd_base -> l_arm_forearm -> l_arm_elbow`; `l_arm_wrist` is another elbow
  child. Hardware is NOT directly parented to the wrist. `cog_gun` is a right-
  wrist child. Blindly pinning wrist/device independently risks reopening the seam.
- PSA Down moves the left wrist by up to 24.2966 UU and the device by 23.0039 UU
  relative to their initial positions; Impulse moves them 12.9157 / 10.8024 UU.
  Right wrist is constant within these TMD sequences: this does NOT prove the
  reported right-gun displacement comes from the TMD animation track itself.
- Read-only measurement script: `tools/reload/audit_dual_hand_animation.py`.
  PSK ancestry, PSA names (not placeholder PSA parents), row-vector local @
  parent, native Y reflection. Source bytes remain private/ignored.

Archived `deployment/deploy-backups/20261024-accepted-hud-36081c3d/view_matrix.log`
contains material-0 final wrist discrepancies, but lacks the relevant shared
L2/Square/R2 transition captures. Old eight periodic samples can expire during
idle. They cannot distinguish a native animation change from a different ride,
missing source draw, or a different controller sample at the failed action.

Hypothesis: native shared presentation state leaks into the controller-owned
rendered assembly. Falsifier: action-correlated source/local/ride matrices show
stable controller-relative positions despite the physical displacement; then
investigate another submitting draw/resource, not the original skeleton.
No new guessed offsets or pose corrections were added.

## Implemented discriminator

`[Diagnostic] DualHandPoseProbe=1`, default 0. Independent of HandAnchorProbe and
of the Seeker enable flags. This is a diagnostic overlay, not a full INI.

- Read-only exact body signatures for Centurion, Valkyrie, Volk, Kasimov and
  Seeker; excludes their secondary attachment palettes. Exact TMD material 0
  and material 1 signatures, original source and actually bound resources.
- TMD mat0 slots 1/18/19 = bones 9/26/51 (left wrist/forearm/right wrist);
  mat1 slots 3/2/5 = bones 27/8/41 (base/elbow/sleeve_fix). Gun root slots reuse
  existing measured body policies. Signature alone is not resource identity:
  source/bound IB, VB/stride/offset and shader addresses are also recorded.
- Record before FgHidden, after visibility, and before the actual stereo draw.
  Capture source and final palettes/local, actual ride, both controller deltas,
  hand pose time, tracked mask, handedness, current weapon, hands/support/action
  state, independent palette count, planned route/faces, draw HRESULT and IB/
  hand/reload-palette restoration results. Zero/missing routes remain explicit.
- Actions are raw existing XR samples (L2=1, Square=2, R2=4), NOT proof that the
  native power executed. The sampler reuses existing polling, does not inject input.
- Maximum 16 state/action/weapon changes, arrival + 250ms settled sample, up to
  6 records each: **192 records/session maximum**. Unchanged idle cannot exhaust
  the budget. One render thread, state latched per frame, one source attempt per
  part/frame, duplicate/stale eye records rejected. No GPU readback, fences,
  allocation in the hook, UObject traversal or new detours.
- Formatting/logging occurs at Present, not in each draw. Getter-returned COM
  references are released immediately. No data is zeroed/read when the flag is
  off except the normal flag/false-branch cost; fixed stack storage is reserved.
- Diagnostic HRESULT success is NOT pixel/headset proof. Planned face totals
  describe the selected dispatch; they are not GPU-content validation.

## Next evidence and correction boundary (MAIN)

Keep the current accepted visual/input settings. Enable only the diagnostic key.
In normal gameplay with TMD and a held weapon, keep both controllers steady:
idle, L2/release, Square/release, R2/release, then near-weapon left Grip/release.
Leave about one second between actions. No typing, pauses or tab switching in
headset. No need to repeat Seeker acquisition or change saves for this capture.

Required receipt: `dual-hand-pose frame=... actions=...` for the actions and
released state, both eyes for part 0 (glove), part 1 (device when visible), and
part 2 (gun). Missing stage 1 = source absent/outside eligible foreground;
stage 1 without 2 = visibility exit; stage 2 without 4 = later exit/read failure.
Use bound/restore status to distinguish missing geometry from a displaced pose.

Reconstruct GPU rows with `armbind::FromGpu`; actual joint = original native
bind * skin, world = joint * local * recorded ride. Compare the same frame/poseTime,
not different weapon epochs. Analyze (a) right wrist versus weapon root,
(b) left wrist versus forearm/device, and (c) differences in recorded controller
transforms. Preserve the measured native child recoil and device articulation.
Only then choose the smallest common-parent correction; do not freeze cog_gun,
remove entire switch functions, or independently offset glove/device.

## Verification

- Action/state budget x86 /O2 tests: PASS (OFF, idle, entry/settled, stereo,
  duplicates/stale frames, time wrap, 16-transition ceiling, L2/Square/R2,
  support and ownership changes).
- Dual-TMD suite: 19 C++ programs + existing Python wiring/preview checks PASS.
- TMD runtime regression suite: PASS, including own-process D3D9/Ex SVTG and
  state restoration; existing native hands/reload/aim/WeaponLocker checks.
- OLED D3D11 actual shader/CPU comparison: PASS (all presets <=1 LSB).
- Compiled WeaponLocker x86 factory literal `[eax+20Ch]`: PASS.
- x86 PE32 /O2 build: PASS; source/commit and final DLL SHA are in the adjacent
  validation receipt. Existing baseline static-analysis warnings remain.
- Steam, OpenXR/game runtime, physical PSVR2: NOT RUN.

GO for MAIN diagnostic integration only. NO-GO for claiming the requested action
independence repaired. The previous Seeker implementation remains in the branch;
this follow-up must not be advertised as a new hand-animation fix.
