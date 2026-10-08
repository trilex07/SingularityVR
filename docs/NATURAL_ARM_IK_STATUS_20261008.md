# Natural arm tracking candidate — 2026-10-08

The native-hand path previously transformed the hand/finger palette as one rigid
block. That preserved fingers but left the shoulder/elbow chain in the game's
constant bent pose. This continuation adds an opt-in, bounded two-bone adapter.

## Runtime modes

`[Render] ArmIKMode`:

* `0` — existing native animation path (default, rollback-safe).
* `1` — constrained TMD arm IK. The right/left controller transforms already
  used by the weapon/hand path are the wrist targets. The solver preserves
  upper-arm and forearm lengths, transports the elbow bend plane, clamps an
  unreachable target, and leaves finger/unknown palette slots on the existing
  independent-hand path.
* `2` — TMD gloves-only fallback. When the TMD is raised, only the audited arm
  section (`7444`, `startIndex=13206`, `primCount=7704`) is suppressed; the
  section-0 glove/hand draw remains. This is a fallback for a visual test, not
  the default candidate.

Mode 1 is limited to the verified TMD section maps:

* section 1: left shoulder/forearm2/elbow = slots 0/1/2; right elbow/forearm/shoulder = 12/13/14;
* section 0: left wrist/forearm = slots 1/18; right wrist = slot 19.

No identity-index assumption is applied to other meshes. Failures leave the
original palette unchanged.

## Desktop verification

The deterministic tests pass after the adapter was added:

* `test-reference-integration.exe` — bind reconstruction, arm IK reach/clamp/
  elbow continuity, new left/right static TMD adapter and finite palette writes.
* `test-production-runtime.exe`, `test-vr-interaction.exe`,
  `test-vr-motion-sim.exe`, `test-vr-transition-sim.exe`,
  `test-vfx-eye-scale.exe`, `test-foreground-boundary.exe` — PASS.
* `validate-runtime-policy.ps1` — PASS.

A Steam desktop/XR probe loaded the candidate successfully. The valid runtime log
is `C:\Users\Trilex\AppData\Local\SingularityVR\view_matrix_2026-10-08_22-24-28.log`:
it records `arm IK mode=1`, `OpenXR ready`, the first frame submitted to the
headset runtime, desktop automation, and the TMD being acquired. It does not
contain a per-draw IK counter or a headset screenshot, so it proves loading and
runtime stability, not final elbow appearance. The Steam install was restored
from the probe backup and is back on the prior safe DLL
(`86FA0B0EFC1D62DA7E1303CF145DB985CE31EE3D43C954D7457CE141B4CCF930`).

## Candidate

`deployment/arm-ik-candidate-20261008/` contains `ArmIKMode=1`, independent hands
enabled, VFX smoke/screen flash disabled, and desktop automation disabled.

* DLL: x86 PE32 (`Machine=0x014C`, `OptionalHeader=0x010B`)
* SHA-256: `B627E5C594577D2411B5BCD0C317FE9BB31FC5FE9D64F85188DF0EC48A316AD3`

This candidate still needs one headset observation to judge shoulder placement,
elbow bend and whether the glove-only fallback is preferable.
