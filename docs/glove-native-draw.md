# PR #30: original UE3 hands, not another albedo approximation

The second physical review accepts size but rejects material and the missing
short forearm. Earlier GLB candidates are NOT appearance-accepted. After two
unsuccessful appearance corrections, investigate the producer rather than
tuning tint, UV scale or another substitute texture.

## Evidence and hypothesis, before implementation

- UEViewer exports `inview_gloves_gauze` with `inview_glove_d`, `_n`, `_s`,
  detail normal and parent `Mat_CharacterMat` / `Base_CharacterMat`. The old
  renderer supplies only `_d` to fixed-function D3D9. Exact diffuse bytes do
  not reproduce that material. Offline export used the private lab copy,
  never Steam. Missing lab TFC prevents texture decoding in that export;
  the material references themselves were decoded successfully.
- Native noTMD section 0 has 4830 triangles / 2924 vertices. All 16512 cooked
  indices including section 1 round-trip exactly to ActorX wedges after its
  winding reflection. Existing controller anchoring already transforms this
  native draw and preserves the game's shaders and material.
- The 2918-face wrist/finger-dominant cut excludes forearm-weighted cuff faces.
  Left cuff weights include `tmd_base`, right cuff includes `r_arm_forearm`;
  a wrist/finger-name-only filter cannot retain them. Measuring against the
  wrist→elbow axis distinguishes actual forearm from a decorative end-cap.

Hypothesis: retaining the original native VB, vertex declaration, shaders,
textures and light constants, selecting original cuff faces, and changing only
the known finger skin palette will remove the material mismatch without a
hand-size adjustment. Offline preparation must prove source index identity,
closed cuff topology and that all retained skin contributors belong to the
existing native palette. Pose changes must not reach a held weapon or TMD.

Falsifiers: native mode still looks different from the held native glove in
the same light; no cuff lies anatomically behind the wrist; either hand changes
size, loses controller rotation, or held-gun pose changes. A successful build
or draw count cannot disprove these physical failures.

The new mode is default OFF. MAIN alone owns deployment and physical testing.
No claim of L3/L4 acceptance is made here.

## Implemented native candidate

`[Render] GloveNativeEnabled=0`, startup-only, takes priority over the old GLB
flag. No diffuse/normal/specular files are loaded by this mode: the engine's
currently bound original hand material supplies all of them. Original UVs,
tangents, weights, VB, vertex declaration, shader programs and lighting remain
bound. Only indices and the known 35-slot free-hand palette change. Native
IB and palette are restored after the existing `StereoPair` sequence.

The private `native_hands.svni` contains original selected indices and authored
finger-joint rotation deltas, not an approximation of the glove surface. It is
generated from the lab PSK/cooked pair without needing the earlier rigid GLBs.
The proximal cut is 3.5 source UU behind the anatomical wrist→elbow axis; it does
not rescale the hand. At default 75%/52.5 UU per metre the boundary lies about
3.6–4.9 cm behind the left wrist and 1.3–5.0 cm behind the right wrist (ragged
authored gauze topology, not a fabricated straight sleeve). 1782 left +1912
right original triangles, 98 closing cap triangles =3792 triangles total,
one native indexed submission per eye. Caps connect original skinned rim
vertices and inherit their UV/normals; the interior cap is new closing geometry,
not claimed to be an original game surface.

The cuff uses the same native wrist transform for every original contributor,
not an elbow animation or stump collapse. Open/fist uses real finger parents
and bind positions, with artist-authored rotations (not claimed native game
animation), per-hand analog deadzone and rigid quaternion interpolation. The
existing wrist/controller scale is retained. Held firearm, support grip, TMD,
world, shadow, pause/menu and cinematic paths do not consume the new pose/cut.
This candidate covers the measured pre-TMD noTMD rig only.

L1_PASS: existing 284 GLB checks, nine geometry tests and native hand regressions;
new fake-D3D IB binding/refcount/failure restoration tests, malformed-cache
rejection, native-palette rigid 75% scale at 101 analog levels, separate hands,
wrist/cuff and unused-register preservation, release→open, two Python native
source/quaternion/topology tests. Both cuffs are welded closed at open/fist;
axial CPU rays hit cap at 6.84/6.95 UU rather than the far hand at 19.38/19.16.
Synthetic native pose CPU =0.0051 ms; not GPU timing or no-hitch proof.

Private prepared data: `build/gloves/native-ue3/`, including topology-only PNG,
source hashes, receipt and axial rays. Cache SHA256:
`B4564A69AF904DDE7BE9077EBBC96677D93B42E104DEDD93F94B6BF8ACE46C04`.
No game-derived binary/texture is added to Git.

```powershell
python tools/gloves/prepare_native_hands.py --psk <lab>/ch_inview_noTMD.psk --cooked <lab>/RvGame.decompressed.xxx --output build/gloves/native-ue3
python tools/gloves/test_native_hands.py --psk <lab>/ch_inview_noTMD.psk --cooked <lab>/RvGame.decompressed.xxx --output build/gloves/native-ue3
tools/gloves/test-gloves.ps1 -NativeCache <absolute>/build/gloves/native-ue3/native_hands.svni
spikes/view_matrix/build.ps1 -GameBinaries <absolute>/build/never-deploy
```

MAIN: integrate PR #30 into the current combined source, rebuild and verify
its full file manifest; do not replace a newer combined DLL with this old-base
standalone build. Put the single new cache at
`Binaries/vr-assets/hud/katorga/glove-animation/native_hands.svni`; enable only
`GloveNativeEnabled=1`, with current independent-hands/isolation-2 settings.
The old GLB caches/textures are not used in this mode. A startup `CPU ready`
line does not prove a native draw: check `scoped original-material draw=1`
and still validate actual stereo content. Offline cooked index identity is
proven; live VB/material identity and perceived result remain L3/L4 debt.

Three-sentence physical test, no switching to chat: Before obtaining the TMD,
rotate both empty hands in roll/pitch at the accepted distance/size and inspect
the same native glove material plus short forearm, including the cuff opening.
Close/release each grip separately, then pick up/holster the pistol and compare
appearance in the same light without changing scale.
Pause/resume and exit normally, reporting material/cuff, grip, held-gun and
stereo/FPS regressions separately; MAIN restores the verified safe install.
