# Knife + original glove: one grip assembly, separate native materials

SIDE, no Steam. Isolated integration branch combines PR #30 `423c2bb` and
PR #31 `cf42caf`, not a merge to main. Those independent branches remain intact.

## Observation / hypothesis / falsifier before implementation

The knife is currently attached directly to the XR aim pose, while the native
glove uses a reconstructed wrist and independent finger palette. No shared
mount exists. Worse, `knife_renderer.h` clears a private depth surface in its
final overlay: it cannot let the already-rendered fingers occlude the handle.
Moving only the knife pivot cannot fix that depth-order mechanism.

The measured arm VS dump `research/native-hands-20261007/shaders/0-VS.txt`
declares `ViewProjectionMatrix` c0 and `LocalToWorld` c230, skin c5. Its final
position instructions multiply skinned position by LocalToWorld rows, then
ViewProjectionMatrix rows. Resolve registers from CTAB at runtime, not fixed
addresses/register guesses. The same route is present in captured VS #1.

Hypothesis: fit a knife-specific original-rig finger pose + rigid knife mount
offline, then consume the SAME actual native wrist/local/eye matrices and depth
surface inside the existing native hand stereo draw. Keep glove UE3 material;
keep knife's original diffuse/UV. A permanently welded mesh is unnecessary and
would make releasing it and preserving separate materials harder.

Falsifiers: exact fitted meshes still intersect offline; native hand and blade
drift relative to one another during wrist rotation; fingers do not occlude
handle from both eyes; glove material/size changes; release leaves fingers
locked. New mode default OFF, bounded counters, no new hooks, native palette,
IB and all GPU state restored in the same draw. Initial controlled scope is
the measured pre-TMD noTMD rig with firearm holstered/unheld; held-gun and TMD
poses are not guessed or overridden. MAIN owns live state/depth and PSVR2 proof.

## Implemented assembly

`knife_hand.svng` contains two measured knife-to-bind-component mounts and five
finger fractions per hand. The original native hand VB, UVs, material, size
chain and cuff remain unchanged. Both mounts use the actual modified wrist
skin matrix, actual native LocalToWorld and the current GPU per-eye projection
inside the existing StereoPair. CTAB identifies the colour camera symbol;
captured shadow shader #2 has no such symbol and is not a colour owner.

Inside that native colour draw, replay the original glove into a private depth
surface with all MRT colour masks OFF, then draw the original textured knife
against that depth. Never clear/write the world's original DS in the added
replay. ALL state plus RT/MRT/DS/viewport are restored, using existing raw
driver callbacks, without new hooks. The old held-knife overlay is suppressed
in joint mode. Added cost is one glove depth replay + one knife draw per eye;
mesh scratch is preallocated at startup, not loaded during swings. Near/far/W
homogeneous clipping preserves perspective-correct original UVs.

Strict dependencies: `[Render] GloveNativeEnabled=1`, `KnifeHandGripEnabled=1`,
`[Input] HipKnifeEnabled=1`; keep other accepted INI settings, including the
native tracked-hand/isolation settings. `GloveRendererEnabled` is not required:
the native mode takes priority over that older GLB renderer. All new modes
remain OFF in the repository default INI.
Missing/malformed caches, unsupported projection, MSAA or GPU failures disable
the knife experiment rather than substituting an unrelated glove/gun pose.
This initial mode is **pre-TMD, free noTMD rig, firearm holstered/unheld**.
Simultaneous pistol + offhand knife is not implemented. Knife lighting remains
the previous original-diffuse-only path, not UE3 normal/specular reconstruction.
Combat remains native camera-trace RB melee, not physical blade-tip collision.

## Offline evidence, not Steam/PSVR2 validation

- L1_PASS: 147 interaction checks, 360000 synthetic updates, existing holster
  and native hand-anchor regressions, fake D3D state/refcount/failure tests.
- Seven knife parser/UV tests + three joint triangle/distance/containment tests.
- Actual original-rig C++ replay matches independent fitted geometry: 2218
  hand vertices +1324 knife vertices, both hands, tolerance 0.00015 UU.
- Release/non-holder tests: optional finger pose does not latch state and
  leaves the opposite hand's ordinary analog pose unchanged.
- Captured native colour VS #0/#1 and shadow VS #2 CTAB distinction PASS.
- Independent cached-assembly validation: zero surface crossings and zero knife
  vertices buried inside either glove; closest per-digit contact gaps are
  0.0911–0.1535 UU (about 1.3–2.2 mm at default scale).
- CPU software depth preview of both original diffuse maps inspected from
  four angles per hand; this is NOT a screenshot or proof of live GPU depth.
- Existing glove regression suite: 284 checks, nine Python tests, original
  native pose/IB/state guard regressions PASS. No runtime FPS claim.

Fitted fractions thumb/index/middle/ring/pinky: .825/.525/.475/.450/.725.
The deterministic bounded fit rejected its first intersecting result. These
are private asset fitting data, not guessed game-memory offsets.

Private output: `build/knife-hand-grip/prepared/` contains the 200-byte cache,
report, validation, golden replay and `joint-uv-depth.png`. Nothing game-derived
is committed. Cache SHA256:
`891D8243843A72FBD2F0180CB1703DB33ACF1995C820FE2B538070173C974E9C`.
Input identities are retained in `grip-report.json`.

L2_PASS: clean source `2db0702eb2d3018aa386b4583a5b4f36f60bc283`, `/O2`,
x86 PE32 (014C/010B), instrumentation OFF. DLL SHA256:
`4849DD0D18117353EBF6908EED6F68CFCC313D41576C2F1FF6685A41AB769807`.
Provenance records an empty working-tree status. Later receipt-only changes
do not alter the compiled source. Local combined handoff directory:
`C:\Users\Trilex\vr-mods\SingularityVR-knife-hand-grip\build\knife-hand-grip\candidate`.
It contains the DLL/provenance, four private caches, logs and CPU preview.
Runtime depth/eye matching, swing tracking, Reset recovery and performance are
UNVERIFIED until MAIN L3/L4. Existing unrelated compiler warnings remain.

Reproduce using offline exports, never Steam paths:

```powershell
python tools/knife/fit_hand_grip.py --psk <ch_inview_noTMD.psk> --native <native_hands.svni> --knife <knife.svrk> --output build/knife-hand-grip/prepared
python tools/knife/validate_hand_grip.py --prepared build/knife-hand-grip/prepared --psk <ch_inview_noTMD.psk> --native <native_hands.svni> --knife <knife.svrk> --hand-diffuse <inview_glove_d.png> --knife-diffuse <knife.svrt>
tools/knife/test-knife.ps1 -PreparedMesh <knife.svrk> -JointCache <knife_hand.svng> -NativeCache <native_hands.svni> -Golden <joint-golden.bin> -CapturedShaders <captured-shader-directory>
spikes/view_matrix/build.ps1 -GameBinaries <absolute-offline-never-deploy-directory>
```

## MAIN handoff / three-sentence headset test

MAIN must integrate this combined PR (includes #30/#31) onto the actual current
accepted branch and rebuild; never replace newer unmerged UI/localization/HUD
work with this standalone base-main DLL. Stage DLL/provenance plus
`vr-assets/hud/katorga/glove-animation/native_hands.svni` and
`vr-assets/knife/{knife.svrk,knife.svrt,knife_hand.svng}` together. MAIN alone
owns backup/hash/deploy/restore/Steam and L3/L4 acceptance; SIDE touched none.

1. Before acquiring TMD, with the firearm holstered, pick the left-hip knife
   with each free hand in turn and inspect the same original glove/cuff/size.
2. Turn wrist palm-up/down and look at the handle from both eyes: fingers must
   surround and occlude it, with no blade drift, duplicate knife or penetration.
3. Release/regrab, make one slow then deliberate swing at a close target, and
   pause/resume: fingers must reopen, native melee must not fire the gun, and
   stereo/tracking/FPS must remain stable before normal exit.
