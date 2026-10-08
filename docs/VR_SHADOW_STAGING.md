# SingularityVR shadow staging work

This worktree is deliberately separate from the production DLL. It records a
safe shadow policy and an offline projection simulator; it does not replace the
running `d3d9.dll` and it does not change the main agent's worktree.

## What the reference repositories establish

### `references/vrframework`

`src/fh5vr/Fh5Adapter.cpp` gates the upstream camera producer on the camera's
near/far arguments. The observed gameplay camera is near approximately `0.1`
and a large scene far plane; the shadow/reflection/UI producers use different
values. Only the gameplay camera receives the per-eye transform. The adapter's
comment explicitly says that shadow and reflection cameras pass through
untouched.

The transferable rule is **classify before writing**, rather than trusting a
constant-register number that is reused by several passes.

### `references/Dishonored-VR`

The flicker notes document that the same view constant can be uploaded for
ordinary scene draws and non-eye passes, including shadow depth. The validation
requirements therefore reject a native-position copy in light or shadow. The
performance notes classify shadow maps/captures by their render target and keep
them unshifted; they also keep conventional shadows as the public default.

The transferable rule is **render-target/pass identity is stronger than a
matrix-register hit**. Splitting a shadow map into eye halves creates two
squashed maps, which then sample as large blocks or eye-dependent rectangles.

### `references/KHARVOX`

The archived SFS history distinguishes mono/shared shadow passes from camera
passes and keeps light-space projection unshifted. Its shadow-history code
commits only after both eye producers/consumers are complete, validates atlas
shape/layout, and refuses an incomplete left/right pair. `NativeSharedShadowPolicy.h`
also requires same-size compatible tables before pairing them.

The transferable rule is **preserve native light-space state first; only share
or replay a shadow atlas after same-frame ownership and layout are proven**.

## Current SingularityVR failure mode

Production `spikes/view_matrix/d3d9.cpp` currently exposes:

* `Render/ShadowMode=0`: split the sampling draw like scene geometry;
* `Render/ShadowMode=1`: draw the sampling receiver once with the native matrix;
* `Render/ShadowMode=2`: drop the sampling receiver draw.

The production comments already record the observed failure: splitting a shadow
map or a shadow-dependent scene pass into SBS halves corrupts the map. `ShadowMode=2`
therefore removes the visible shadow entirely. This staging branch does not
change that setting in the installed DLL; it provides the evidence-backed
replacement path for the main agent.

## Staged policy

`spikes/view_matrix/shadow_pass_policy.h` is a pure header with no D3D calls.
It classifies a pass from already available evidence:

1. shader shadow/depth-only evidence is `ShadowLightSpace`;
2. a perspective, scene-sized, eye-viewport camera with near `0.06..0.20`
   and far `>2000` is `GameplayEye`;
3. orthographic/non-eye passes stay UI/post/unknown and are conservative mono.

The policy then selects:

* `StereoEye` for the gameplay camera;
* `PreserveNativeShadow` for a light-space shadow pass;
* `MonoShadowReceiver` for a gameplay receiver sampling the projected shadow
  texture until a native light-space capture/reprojection is available;
* `DropDiagnostic` only when the caller explicitly requests the diagnostic
  drop switch.

Unknown or non-finite evidence never enables stereo. This makes it impossible
for a stale eye state to leak into the next shadow draw.

## Offline shadow renderer preparation

The same header contains a deterministic directional-light view/orthographic
projection and atlas UV/depth mapping. It is a simulator only. A live custom
renderer still needs the game's actual light matrix, cascade/atlas layout,
depth format, comparison sampler and receiver shader contract. Inventing those
values in the live hook would make a plausible-looking but invalid shadow map,
so the production integration is intentionally deferred until a bounded draw
census captures them.

## Tests run

`tools/reference-tests/shadow/run-shadow-tests.ps1` compiles an x86 PE32 test
with MSVC `/O2 /std:c++17 /W4 /WX` and checks:

* gameplay/shadow/UI classification;
* explicit drop versus native-preserve behavior;
* `LEFT_EYE -> SHADOW -> RIGHT_EYE` state isolation;
* exact light-space matrix invariance under a simulated HMD translation;
* directional projection, atlas bias, finite UV/depth values;
* identical atlas coordinates for both eye observations of one world point.

The test passed on 2026-10-08:

```
shadow-policy tests: PASS (classification, matrix invariants, stereo sequence, atlas simulation)
```

## Handoff to production

The main agent should integrate this in two small steps after a headset-free
draw census:

1. feed the existing scene-target/shader/near/far observations into
   `PassEvidence` and use `Decide` before `StereoPair` modifies constants;
2. leave `ShadowLightSpace` on the original draw/matrix and use
   `MonoShadowReceiver` while the sampled map is bound.

Only after logs show a stable native light matrix and atlas ownership should a
second branch add actual shadow-map replay. No Steam deployment was performed
from this side worktree.
