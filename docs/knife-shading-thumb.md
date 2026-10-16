# Knife lighting and separate thumb poses

SIDE follow-up to physical feedback: the joint mount/scale is accepted, knife
is too bright, thumb enters other fingers rather than resting on the handle.
Empty fist also needs a distinct thumb position. No Steam access/deploy.

Base: origin/main `fb73156`, fast-forwarded to MAIN's tested integration
`aef042f` (joint knife PR #32 + Bink stills). Those inherited changes are not
new authoring here. MAIN should cherry-pick this follow-up onto its current
integration, never replace newer work with an older standalone DLL.

## Mechanism, hypothesis and falsifiers

* Existing knife colour draw selects diffuse with lighting disabled. It has no
  normal/specular input and writes alpha=1 into the native scene colour target.
  This is not the original UE3 material, regardless of original mesh/texture.
* Captured native hand VS #1/PS #1 prove named LightDirection, CameraPosition,
  WorldIncidentLighting, SHBasisCubeTextures, LightColor and AmbientColorAndSkyFactor.
  The colour PS ends with `MinZ_MaxZRatio.x / clip.w + .y` in alpha. A constant
  alpha cannot reproduce that depth encoding. This is structural evidence, not
  proof it caused every observed shadow artifact.
* Original `Mat_Knife` export identifies diffuse/normal/specular maps, DiffusePower=2,
  specpower=50. New controlled material uses these inputs and the native scene
  lights instead of a guessed brightness multiplier.
* Old thumb base aims at middle-finger base. Original fitting checked knife
  intersections only: empty fist has 148/149 measured thumb/finger triangle
  crossings; old knife pose has 61/60. Five isolated poses still crossed during
  interpolation. The offline path therefore refines actual failing intervals.

Falsifiers: new light mode never matches actual colour shaders; depth alpha
does not match native encoding; brightness/shadows remain wrong in matched
draws; thumb still penetrates or rests away from handle; original cuff/scale,
opposite hand or pistol changes. Do not compensate unknown shaders by guessed
registers. Capture their identity before extending scope.

## Independent experiments (default OFF)

`[Render] HandThumbPoseFix=1` reads `hand_thumb.svnt` v3 at startup. It contains
an adaptive normalized-quaternion path for empty fist and an independent knife
endpoint, for each original thumb rig. Only thumb rotations change. Other four
finger fractions, knife mount, native VB/UV/material/cuff/scale and wrist chain
remain unchanged. Invalid/missing cache keeps the old pose. Runtime only samples
precomputed keys: no collision solver, asset load or allocation per grip update.

`[Render] KnifeSceneLighting=1` loads original normal/specular caches and tangent
frames, and compiles our HLSL at startup, outside render hooks/DllMain. This is
a scene-lit approximation, NOT the complete original Mat_Knife graph: environment
reflection/rim/wetness and a new shadow caster are not implemented. It reuses
the original SH basis cube textures, native SH/direct/ambient values and exact
native depth-alpha encoding in the original HDR target. Albedo sRGB is decoded;
normal/specular/SH are data. Existing UE3 shadow reconstruction is untouched.

Only an exact captured VS+PS pair enables this light mode:

* VS 2348 bytes, FNV64 `37754B7CC68D8D59`.
* PS 4696 bytes, FNV64 `85D66574CA9E9C00`.

Up to eight AddRef-owned pairs are classified once; Reset releases them. Unknown
pairs keep the previous albedo path and report `sceneLit=0` in the bounded first
four joint draw notes. `sceneLit=1` is execution evidence, not pixel acceptance.
Both modes require the existing native/joint knife features; docked knife lighting
is unchanged. Test thumbs and lighting separately in MAIN-owned sessions.

The joint draw still uses private shared finger/knife depth, original eye VP and
ALL-state plus RT/MRT/DS/viewport restoration. Constant uploads bypass the camera
classification hook. SYSTEMMEM -> DEFAULT texture upload remains D3D9Ex-safe.
No native world/foreground depth clear, game engine write or new vtable hook.

## Reproducible offline preparation

Use explicit offline exports, never installed Steam paths. Output is private
game-derived data and must not be committed or redistributed.

1. `tools/knife/fit_thumb_poses.py --psk ... --native ... --knife ... --grip ... --output ...`
2. `tools/knife/refine_thumb_path.py --prepared ... --psk ...`
3. `tools/knife/prepare_knife_lighting.py --mesh ... --normal ... --specular ... --output ...`
4. `tools/knife/preview_thumb_poses.py --prepared ... --psk ... --hand-texture ... --knife-texture ...`

Use `hand_thumb_refined.svnt` as the deployed `hand_thumb.svnt`, NOT the rejected
v2 five-key fit. v2 is deliberately rejected by the new runtime. Input native
SVNI, knife SVRK/SVRT and accepted mount SVNG stay byte-identical.

## Evidence and remaining gates

L1: 11 free-hand keys; 201 FLOAT32 grip samples per hand with zero measured
thumb/finger crossings. Knife endpoints have zero full hand/knife crossings and
thumb handle gap about 0.13 UU. This is sampled mesh evidence, not a continuous
collision proof or headset judgment of anatomy. CPU original-rig replay covers
13308 vertices across 12 hand/mode profiles; 101 analog palette checks preserve
non-thumb/opposite-hand matrices. Original joint golden and state/upload/holster/
tracking regressions remain mandatory. Our SM3 shaders compile offline; malformed
caches, tangent orthonormality and native depth-alpha arithmetic are tested.

L2_PASS: clean source `230c62e3bbb3275989387b1056155e8669e4b227`, x86 PE32
(014C/010B), `/O2`, instrumentation OFF, manifest working-tree status empty.
DLL SHA256 `E71EF4E18ACF99418AC50F6A2E9BB0C294C2886559582ABB2F6EAD8DE8F086A2`.
Later receipt-only commits do not change compiled source. Existing unrelated
compiler warnings remain. Optional old GLB Python checks skip two unavailable
asset cases; native-rig, joint replay and new material/pose gates above pass.

Private combined candidate (DLL/provenance + dependent original/new caches):
`C:\Users\Trilex\vr-mods\SingularityVR-knife-shading-thumb\build\knife-shading-thumb\candidate`.
It intentionally contains NO INI to overwrite user calibration. New cache SHA256:

* `hand_thumb.svnt`: `430DA39EE16B00FA254DD5579EF49A3BC04514B37B8C93614E91A4F70183E8E7`.
* `knife.svnl`: `CC52EBD38F77F86D5BDE3F4763057AA33F461A57B1730F46AB0CAB27FA09AAE2`.
* `knife_normal.svrt`: `B711D8EC97744080F7A742782F8867043F5D19E6A4D02BBAD535309E1B9C99C1`.
* `knife_specular.svrt`: `BDC06EB79C91CC18FC8D9272A817C47E82380E33766588C0D46DAECE05822802`.

Accepted input native SVNI, knife mesh/albedo and joint mount caches retain the
exact hashes in `docs/knife-hand-grip.md`; they were not edited by this work.
L3/GPU output/lighting match/performance/Reset in-game and L4 PSVR2 are UNKNOWN.
No claim that all shadow defects are fixed or that the knife casts a new shadow.

## MAIN physical test, after verified integration/build/deployment

1. With lighting OFF and thumb fix ON, slowly close/release both empty fists,
   pick/release the left-hip knife with each hand, inspect thumb on the handle
   from both eyes and rotate wrists; size, cuff and accepted grip must not move.
2. In a separate session enable lighting, inspect knife/glove in the same bright
   and dark places, rotate hand and head independently; MAIN checks `sceneLit=1`
   before judging illumination, depth/shadows and both eyes.
3. Release/regrab, draw the pistol, pause/resume and exit normally; original pistol
   grip, shadows, stereo and FPS must remain stable. Stay in headset until exit.
