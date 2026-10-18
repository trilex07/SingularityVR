# Native camera comfort and menu-resume hitch (2026-10-18)

Status: L1_PASS, L2_PASS; native hook and cache eligibility correction implemented.
Runtime/physical acceptance pending. This is not proof that all FPS dips are gone.

## Evidence and hypothesis before patching

PID31668 rejected shader-space translation stabilization: shake persists and
hands flicker. That path offsets the view but not the controller anchors. Leave
`VrTranslationStabilization=0`; do not recycle its candidate.

Offline inspection of the owned plaintext code capture proves a different seam:
RvPlayerCamera's virtual method at byte slot 0x438 accepts (viewTarget, dt), ret8.
It computes/interpolates mCurrentPOV, copies it into viewTarget+8, invokes camera
modifiers through slot0x428, then adds the previous mCurrentShakeOffset to the
outgoing location. The modifiers continue ticking; their new offset is cached.
Thus reading mCurrentPOV in a render hook does NOT read the final shaken camera.
Camera style1 selects the pawn-eye path, style2 follow, style3 death; camera actor
blend and overrides are separate paths. Reflection must corroborate field offsets
before enabling the experiment. The complete method fingerprint must match.
Caller inspection corroborates this: ACamera update calls slot0x438 with its
ViewTarget at+0x348, then consumes location/rotation/FOV into CameraCache at+0x328.
Pending target at+0x374 is separately updated and blended; exclude that entire
transition, not merely the second invocation. No correction at Present or in
GPU constants can replace this before-cache seam.

Hypothesis: stabilizing the native first-person output BEFORE view/frustum creation
keeps world/hand origins coherent, unlike post-hoc shader translation. For the
local live pawn in style1 only, use pawn world location + reflected BaseEyeHeight
and the pre-modifier rotation; publish the same location to mCurrentPOV (the
existing hand anchor source) and the outgoing POV. Keep native FOV, modifier
timers and all gameplay calls. Do not smooth HMD or controller poses. Exclude
cinematic, menus, external camera blending, overrides and unrelated targets.
Falsifier: the eligible/applied counters advance but walking still shakes or
hands flicker; do not stack a shader-space compensation on this path.

FPS: one native DirectInput discovery consumed520.3407ms in PID31668. A nearby
120-frame window averaged15.418ms;520/120=4.336ms explains its excess over11.11ms
(correlation, not proof of all dips). Input cache eligibility currently includes
menu class. Menu transitions invalidate an unchanged device topology. Remove
that eligibility term while preserving native callback replay, VR/pad-off
passthrough, identity/topology invalidation and owner-thread rules. Falsifier:
resume still rescans with unchanged identity/topology, or native menu input fails.
Keep bounded invalidation attribution in the candidate. Do NOT ignore hotplug.

Previous projector cache fix is retained: PID31668 confirms zero evictions,
but that alone did not remove the physical FPS symptom.

## Offline verification and remaining boundary

- Native x86 calling-convention mock invokes the actual wrapper: original exactly
  once, OFF passthrough, output/anchor matching, native FOV preserved, menu,
  cinematic, death, wrong target, camera blend/override and pending-target isolation.
- Math replay:900 synthetic frames of locomotion, stance-height changes and
  native bob/shake; both eyes retain the same hand-to-HMD relative displacement.
  This is synthetic evidence, NOT a replay of measured headset coordinates.
- Input mock: gameplay/pause/options/resume does not rescan unchanged devices;
  hotplug, identity, failure, reentrancy and thread safety regressions remain.
- Projector reconstruction/scope/cache and guarded pawn-reader tests pass.
- Old shader-space walking stabilization is excluded even if both INI flags
  are accidentally enabled. New `Render/NativeCameraComfort` defaults OFF.

The existing sampler also contains77.9/83.3FPS windows WITHOUT another native
enum call. Their remaining residual is not yet assigned to an exact producer.
Do not present the520ms discovery fix as proof of a locked90FPS throughout the
collapse. Retain bounded timing in the same physical camera-fix test; no extra
diagnostic-only headset round, screenshots or mid-test chat switches.

Candidate built from8899f72, x86 PE32 `/O2`, DLL SHA-256
`89966207B8A13959BF27895E38ED78460BE7BC6FE7B4A94AEB6FB8517FD62A14`.
Offline package `build/projector/candidate-native-comfort`; accepted-profile
install `-WhatIf` preflight PASS (no writes). INI SHA-256
`03ADF50D40ECEE4D8F2112E0B3FCC5A315B63A4B5D721AEDB99281B7681661C7`.

Next authorized test (fresh `start` only): closest early-game save, walk/run and
crouch through the affected area, briefly inspect both hands, pause/options and
resume, exit normally. Check the native hook's install/fingerprint, applied and
rejection counters; compare input invalidation reasons and frame windows.
Do not merge/promote until perception and controls pass. Preserve working
projector/muzzle, localization, gloves, knife and reload assets unchanged.
