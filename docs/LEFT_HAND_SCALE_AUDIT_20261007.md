# Left-hand motion and world-scale audit — 7 October 2026

## Scope and result

This pass addresses independent left-hand motion and VR/world scale only. Production was inspected
at `71fa036` on `fix/vr-menu-input-grip-performance`; five pre-existing untracked spike files were
left untouched.

The translation conversion is internally consistent: HMD, left/right controller-relative motion,
and eye separation use one `MetresToUU()` scale and one position map. At the default
`WorldScalePct=100`, this is 52.5 UE3 UU/m. The audit found no controller double scale. No global
scale change is justified by code math alone; in-game scale calibration still needs a measured
physical displacement and a known world/body reference.

True independent left rendered-hand motion is **not fixed by this pass**. The OpenXR left pose and
converted off-hand target exist, but normal rendered arm ownership remains with the game's skin
palette. The only independent palette path is opt-in, gated, and does not consume a full left wrist
orientation or run the arm IK solver. Enabling it by default would claim a behavior that the code
does not currently implement. Live IK also remains unsafe until a same-pose bone-slot map and
whole-chain palette source are proven for the exact draw section.

## Transform map

| Stage | Source |
|---|---|
| HMD pose in metres | `UpdateFromHeadset`, `xrLocateViews`, stores `g_eyePose`; midpoint of two located eyes is `hp` (`spikes/view_matrix/d3d9.cpp`, around 9052–9060 and 9240–9259) |
| Centre/recenter reference | Same function captures `g_centrePos`, `g_centreYaw`, and `g_baseYaw`; L3+R3 only sets `g_recenterRequested`, then clears/rebuilds this common reference (around 8445–8446, 9072–9080, 9120–9190) |
| Position basis + metres-to-UU | `vrpose::TrackingDeltaToWorldUU` in `spikes/view_matrix/pose_conversion.h`; production calls from the HMD, eye, aim-hand and off-hand blocks (around 9267, 9314, 9385, 9416) |
| Camera | HMD result is published as `g_hmdOffset`; camera constants receive it through the camera/view constant handling and translated-world matrix path (`g_hmdOffset` consumers around 10016 and 10271) |
| Left/right raw grip poses | one loop calls `xrLocateSpace(g_handSpace[i], g_xrSpace, handTime, &loc)` for both indices (around 8310–8325); raw positions saved only when position is valid and tracked |
| Left/right controller-relative position | `g_handLastPos[hand] - g_headPosXR`; converted and stored as milli-UU in `g_handOff*` / `g_offHandOff*` (around 9380–9421), divided by `kHandFixed` on read |
| Wrist target / IK / palette | No normal runtime path from off-hand pose into left wrist IK/palette. `PrepareIndependentLeft` (around 13089) is a disabled-by-default exact-mesh prototype called from indexed draw handling (around 14760). `armik::Solve` has no production caller. |
| Weapon | `GunPivotWorld` reads the aim-hand offset; `BuildGunC` composes it with the gun anchor and orientation (around 12950–13080). The left target is not the owner of the right weapon transform. |
| Eye baseline | `xrLocateViews` left/right eye position difference → same `TrackingDeltaToWorldUU` → `g_eyeDeltaUU` → right-eye matrix offset in stereo draw (around 9309–9320; application around 10828) |

The current path uses predicted grip poses for both hands and one predicted time. `AimHand()` and
`OffHand()` swap logical roles according to handedness, but do not alias the two raw OpenXR spaces.

## Coordinate and matrix audit

OpenXR position axes are +X right, +Y up, and −Z forward. The code maps to Singularity translated
world as +X forward, +Y right, +Z up. At zero centre/body yaw, for column-vector notation:

```text
B = [ 0  0 -1 ]
    [ 1  0  0 ]
    [ 0  1  0 ]
```

The actual implementation applies centre yaw first to obtain forward/right, then game base yaw to
obtain world X/Y; Z is up. `pose_conversion.h` implements this exact map. `det(B) = -1`,
`BᵀB = I`, each basis vector has length 1, and basis vectors are mutually perpendicular. The
negative determinant is intentional: preserving semantic right/up/forward axes while mapping
OpenXR’s right-handed axis labels into the game’s documented left-handed UE3 convention requires
one reflection. There is no later arbitrary sign correction in the position path.

The camera and bone code use row-vector affine matrices in relevant places: `skeletal_palette.h`
declares row-vector matrices and bone constants as three output-component rows; `arm_ik::Frame`
also emits its frame in row-vector storage. Position conversion itself is a vector map, independent
of matrix storage. OpenXR quaternions are `{x,y,z,w}`; current heading code extracts pitch about X,
yaw about Y, and roll about Z with the same formulas for both hand indices. This is not a complete
left-wrist quaternion-to-bone-palette conversion: the dormant hand prototype does not consume the
off-hand quaternion to produce a wrist frame.

## Scale ownership

`MetresToUU() = 52.5 * 100 / WorldScalePct` when the setting is positive. The default is 100, so
52.5 UU/m (UE3’s 16 units/foot). The setting is clamped to 50–200 at INI load and in the panel.

| Consumer | Input | Scale | Count | Expected |
|---|---|---:|---:|---|
| HMD translation | metres from eye midpoint minus centre | `MetresToUU()` in helper | 1 | 52.5 UU per physical metre at default |
| Left controller relative to head | metres | `MetresToUU()` in helper; then `×1000` fixed-point store and `/1000` read | 1 physical scale + storage encoding | 52.5 UU per metre |
| Right/aim controller relative to head | metres | same helper and milli-UU encoding | 1 physical scale + storage encoding | 52.5 UU per metre |
| Eye separation | located eye-position delta in metres | `MetresToUU()` in helper | 1 | physical baseline × 52.5 UU/m |
| Recenter translation | reference position in metres; subtracted before conversion | same helper after subtraction | 1 | rigid translation; does not scale relative deltas |
| Weapon anchor | authored UE3 units plus the already-converted hand/HMD offsets | no second conversion on hand offset; explicit `.055 * MetresToUU()` is a 5.5 cm authored attachment offset | 1 for physical offsets | anchor and motion remain in UE3 units |
| Hand storage | milli-UU integers | multiply/divide by `kHandFixed=1000` | storage only | unchanged UE3 distance |
| Skeletal/model geometry | game palette/model units | no `WorldScalePct` conversion | 0 | remains game-authored foreground geometry |

Candidates at scale percentage P: `units_per_meter = 52.5 * 100/P`. At 100%, 1 cm/10 cm/50 cm/1 m
map to 0.525/5.25/26.25/52.5 UU. At 110%, they map to 0.4773/4.7727/23.8636/47.7273 UU. Any candidate
must change HMD, both hands, and eye baseline by the same ratio. The 64 mm eye example is 3.36 UU
at 100%; production does not hardcode that baseline and uses the runtime’s two eye positions.

Scale result: **NO SCALE BUG PROVEN**. The report “too large” cannot by itself distinguish global
scale from first-person mesh calibration. The code shows that weapon/hand mesh dimensions are
game-authored and are not rescaled by `WorldScalePct`; a foreground-only size complaint would be a
model/foreground calibration issue. No known world object or physical movement was measured in this
pass, so that perception classification is not yet proven.

## Deterministic results

The new `pose_conversion_invariants` test runs the production conversion helper for X/Y/Z 10 cm
vectors and a 10 cm diagonal at nonzero centre and base yaw. It verifies translation linearity,
equal HMD/left/right magnitudes, eye-baseline scaling, basis vector lengths, orthogonality, and the
intentional determinant −1. Tolerance is 1e−4 UU for output differences and 1e−6 for unit-basis
properties. `quaternion_conversion_invariants` exercises the production component extractor with
identity, ±90° around X/Y/Z, and a combined unit quaternion; it checks finite angles, identical left
and right extraction, and the resulting Y-X-Z rotation matrix's orthogonality and determinant +1.
Tolerances are 1e−4 for matrix properties and 1e−3 rad near the float-limited ±90° pitch cases.
Results: **PASS**. This validates the shared angle extractor; it does not claim a controller
quaternion is currently applied to the rendered left wrist palette.

At 100% scale, a 10 cm displacement maps to 5.25 UU for HMD, left, and right. 64 mm maps to
3.36 UU. Recenter’s existing tests verify rigid relative-distance invariance and idempotence:
**PASS**. The existing arm-IK tests verify segment reach/clamp and a monotonic extension case:
**PASS**. Existing support-grip tests verify acquisition, hold, release, grace, and weapon-switch
rules: **PASS**. These tests do not constitute a live left rendered-wrist integration test.

## Left-hand ownership trace and skeleton evidence

First lost ownership: after the raw left pose is converted and stored as `g_offHandOff*`, before a
left wrist target is used to author the rendered arm palette. Normal first-person skinning continues
to consume the game palette. The prototype `PrepareIndependentLeft` is gated by
`IndependentHandsPrototype=1`, specific motion/foreground/weapon conditions, specific shader CTAB
layout and specific draw ranges. It computes a palette delta from `BuildTmdC` and `BuildGunC`; it
does not run arm IK and its free-hand wrist rotation is not driven by the left controller quaternion.
Support-grip changes in that prototype do not establish a complete free-left-hand path.

The asset-derived `singularity_bind_reference.h` confirms the listed UE3 reference indices by name:
left shoulder 7, elbow 8, wrist 9, forearm 26, forearm2 46, bicep 47; right shoulder 49, elbow 50,
wrist 51, forearm 85, bicep 86. This is the reference skeleton, not proof that the active draw’s
75-slot c5 palette uses identity slot mapping. The prototype’s per-section left/right slot ranges
are different evidence and do not prove that GPU slots 7/8/9 or 49/50/51 correspond directly to
those skeleton indices.

`arm_bind::BuildArmDeltas` requires the original shoulder, elbow and wrist skin matrices from one
animation pose. `ApplyArmChain` requires a verified per-section bone-to-palette-slot map and restores
the original palette after the native draw sequence. Existing project notes explicitly keep live IK
disabled because the chain palette ownership/source has not been verified. Do not enable runtime IK
until one bounded capture proves those same-generation inputs and output transforms.

## References inspected

| Project/revision | Files/functions | Relevant finding | Applicability |
|---|---|---|---|
| Overlord `11f3005034188894976cb8392122ad166d59f927` | `src/client/component/vr/openxr_input.cpp`, `controller_pose_reference.hpp`, `controller_calibration.*`, `body_pose.hpp`, `camera_rig.hpp`; pose publication and calibration functions | value-owned timestamped hand poses, explicit controller metadata/calibration, explicit units-per-meter, support constraints downstream from independent controller poses | Architecture directly applicable; H2 constants and engine unit values are not |
| vrframework `751863e2076c9c142553c69a73d159f483b2ab5e` | `guides/09-camera-and-coordinate-systems.md`, `guides/12-input-and-motion-controllers.md`, `include/spi/IEngineAdapter.hpp`, `src/fh5vr/Fh5Adapter.cpp` | preserve runtime pose units in core; do conversion in engine adapter; basis conversion/conjugation and action/pose ownership are engine-specific | Adapter ownership directly applicable; FH5 implementation incomplete and its constants are not |
| Singularity | `d3d9.cpp`, `pose_conversion.h`, `recenter.h`, `arm_ik.h`, `arm_bind.h`, `skeletal_palette.h`, `singularity_bind_reference.h` | common position scale is already present; left pose loses rendered ownership before palette; IK helpers exist but are not in gameplay call flow | Exact production evidence |
| REFramework, Starfield2VR, AnvilEngine2VR | Source discussed in local vrframework guides and `VRFRAMEWORK_OVERLORD_COMPARISON.md`; no local source checkout was present in the `references` directory | useful guide-level examples of engine-specific pose adapters and basis conversion | Guide reference only; not independently inspected at source/revision in this pass |

## Remaining readiness gates

- Implement a left wrist pose driven by the current left grip pose, preserving independent right weapon ownership.
- Prove the current draw-section palette map and same-frame shoulder/elbow/wrist inputs before enabling IK.
- Add and pass production-level free/held/released ownership tests against that exact integration.
- Capture bounded raw-left → converted-left → wrist-target → final-wrist values and verify solve count is once per pose/game-frame, not per eye/draw.
- Measure a physical 20 cm head and controller displacement against UE3 values and compare with one known-sized world/body dimension before deciding whether to change `WorldScalePct`.
- Only then deploy and run one PSVR2 check for independent hand, weapon stability, grip release, scale feel, and matching head/controller movement scale.

## Continuation: bounded TMD palette trace — 7 October 2026

Added an opt-in `[Diagnostic] ArmPaletteTraceFrames=0` setting. Zero is the default; values are
clamped to 64. The capture is limited to the exact 7444-vertex TMD draw sections (`startIndex=0,
primCount=4402` and `startIndex=13206, primCount=7704`), only while the verified shader CTAB layout
is bones `c5+225` and `LocalToWorld c230`, the scene target is active, and the first-person mesh
window matches. It stops after the configured number of matching frame tokens. It records a
draw/frame token, shader and VB/IB identities, geometry range/stride, most recent overlapping bone
upload range/sequence, whether the whole 225-register palette was uploaded in that frame, c230, and
selected 3x4 values. It reads before the existing draw-time palette prototype and performs no writes.

The capture is useful for proving section ownership and same-frame coverage, but it cannot by itself
prove a bone identity from motion. Current PSK section maps split the left chain: TMD section 1 has
left shoulder 7, elbow 8 and forearm2 46; section 0 has left wrist 9. Section 0 also has right
wrist 51 and left forearm 26 (slot 18), while section 1 has right shoulder 49 and elbow 50.
The listed TMD maps do not expose left bicep 47 in these sections. The trace must show matching frame, shader, VB/IB,
and LocalToWorld identity between section records before any chain reconstruction is accepted.

To collect runtime evidence, set `ArmPaletteTraceFrames=16` under `[Diagnostic]` in the existing
INI, launch one gameplay session with a visible first-person TMD arm, then inspect
`%LOCALAPPDATA%\SingularityVR\view_matrix.log` for `ArmPaletteTrace`. Compare consecutive matrix
translations/orientations while moving the right weapon and native left arm independently. No live
game capture was available in this pass, so identity by motion, matrix quality, cross-section joint
lengths, controller-to-component conversion, wrist quaternion calibration/order, full IK, support
release at the rendered wrist, and right-arm stability remain unproven.

Added `arm_palette_decoding_invariants` to the production integration test. It verifies the PSK bind
reconstruction convention, GPU 3x4 transpose, rigid current joints, and constant upper/forearm
lengths for a synthetic same-pose palette. The test suite passes. The x86 production translation
unit compiles to an object with `/O2`; no DLL was linked or deployed because runtime palette source
and the complete left wrist/IK path are not yet verified.

## Required production provenance and upstream source investigation — 8 October 2026

Use the existing read-only `SingularityVR-tools/code-tools/build_provenance.ps1` with explicit
`-Repo`, `-BuildDll`, and `-DeployedDll` arguments after building/deploying the implementation.
The required physical-test gate is x86/PE32 plus `BUILD == DEPLOYED: YES`; retain final SHA-256.
The script reports the selected checkout's branch/HEAD and compares binary hashes. It does not
embed or recover a source commit from the DLL, so the fresh build log/source state must separately
connect that DLL to the expected production checkout. Do not recreate the script or use unrelated
performance/GFx tools for this task. No new DLL has been linked/deployed in this continuation.

Static disassembly of the retained live `.text` dump (base `0x402000`, capture
`research/etw/capture-20261006-205508/Singularity.text-memory.bin`) and string/vtable references in
the Steam executable yielded this concrete upstream chain. Addresses are for that executable only;
these are static findings, not validated live hook sites:

- `0x586540` binds the UTF-16 `BoneMatrices` string at `0x17C2AF8` into the shader parameter
  object at `this+0x16`.
- `0x590260` reads a vertex factory's palette data/count at `+0x2D8/+0x2DC`, passes it and the
  bound bone parameter to `0x58BD30`. That helper computes count times 48 bytes and calls RHI
  vtable slot offset `0x114`.
- `0x518EA0` is the D3D9 parameter wrapper: byte offset/count become register offset/count by
  division by 16; it calls the device's vtable offset `0x178` (SetVertexShaderConstantF).
- `0xB6D070` receives fresh dynamic mesh data, replaces `this+0x48`, reads LOD data through
  `this+0x18`, and constructs the section palettes before their uploads. At `0xB6D1B2` the source
  bank starts at dynamic-data `+0x10`; the code uses its pointer at bank `+0x1400` when non-null,
  otherwise the inline bank. Chunk bone-map entries are uint16 values at chunk `+0x1C`, with
  count at `+0x20`. Bone indices multiply by 64, then the code transposes the first three columns
  of each 4x4 source into the destination 3x4 rows (48 bytes). Another factory variant repeats
  this at `0xB6D4D2` onwards.

This provides a possible full-pose source before either section draws, addressing the scheduling
problem that draw-only capture cannot solve. Before installing an override, verify the function
fingerprint/calling convention, dynamic-data bounds, exact mesh UObject identity, generation
ownership, and numerical agreement between this bank and the exact TMD draw palette. Do not assume
the adjacent array count/capacity fields without further evidence, or reuse a previous-frame bank.

Further disassembly narrows that source to `0xB40500`: arguments are output bank, skeletal
component, and LOD index; the function uses component `+0x1E0` for the skeletal mesh,
`+0x218/+0x21C` for current component-space joint array/count (or the parent animation component
at `+0x23C` with its bone map), and mesh `+0xD4` for the inverse-reference array, unless overridden
by component `+0x488`. It multiplies inverse-reference by current joint through `0x4CEE40`, then
stores 64-byte matrices for the LOD's active bone indices. The result is the full skin bank, not
the posed joint bank. Unused bones must not be read as initialized matrices.

The dynamic-data constructor at `0xB7C480` calls this function at `0xB7C503`, before the render
update. Its output bank begins at dynamic-data `+0x10`; bank `+0x1400` is the optional heap pointer,
and bank `+0x1410/+0x1414` are initialized by the constructor. `0xB4055D` resets bank `+0x1410`
before resizing, establishing the count field (dynamic-data `+0x1420`). The previous assumption
that count might immediately follow the pointer would have been wrong because the allocator is
aligned. A future hook must preserve the native calling convention and parent-animation branch.
