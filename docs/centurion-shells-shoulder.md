# Centurion shells and physical left shoulder — SIDE

## Brief / falsifier before implementation

User requests ejected cases plus an independent shoulder-mapping audit.
Existing original Reload removes the whole rigid cylinder, not separate cases.
The offline original package has FX_handgunShellsEject -> mesh type-data ->
shell_rifle and required material shells -> shell_rifle_d. Reuse that asset,
not a procedural brass cylinder. No native particle ABI or inventory write is
inferred from names. New manual-event cosmetic ejection is default OFF, bounded
to six cases, fixed storage, short analytical flight; no collision/audio claims.
Falsifier: no burst before Dumped; once per dump; trajectories stop following
the hand; pause/tracking/weapon/context loss clears them; both eyes agree.

The yaw sign is already correct for OpenXR (+X right,+Y up,-Z forward).
Negative left distance would mirror it. However g_headPosXR is explicitly the
LEFT eye in duplicated stereo, not head centre. Use the valid two-eye midpoint
for this shoulder only; do not change accepted hand/camera/hip transforms.
Retain physical tracking yaw at reload start, latched during the transaction;
without torso tracking this estimates the torso from head direction at START,
not an independently tracked shoulder. Begin facing forward before Circle.
Falsifier: relative to that latched frame the anchor must be exactly22cm left,
12cm below,18cm behind, independent of recenter/game yaw, eye IPD, seated height,
head pitch/roll and later head turns. Grab target and stock preview share it.

L1/L2 gates and exact source/cache/DLL receipts follow after implementation.
ZERO Steam; current MAIN integrates/rebuilds source, never this older SIDE DLL.

## Asset evidence / reproducible private conversion

Validated original decompressed package SHA256
`0534C5C9A5878D280294168BD12711B99CB3948503D4ED53608A0316479D1B18`.
Parsed real object-reference arrays, not text matches:
ParticleSystem743 -> emitter734 -> LOD411/412 -> type-data mesh676 ->
StaticMesh1081 `shell_rifle`; required module -> material272 `shells` ->
referenced texture1104 `shell_rifle_d`. Both LODs agree. This establishes asset
identity, NOT that the native game particle producer was invoked in our mod.

UEViewer static export:105 points /136 UV wedges /100 triangles, zero skeleton
or skin weights. Mesh SHA `0B270E50B804FFB86BAB067B2301DDE69132A3D710C37F76894D67479E452642`;
original DDS SHA `A8379D67D5C5C2F15FC1BDA427AA99A7FEF2D692FBB90E5B0BDA87EFF355A6BD`.
The converter pins those three inputs, preserves all UV/triangle/point geometry,
translates the authored centre for spinning and round-trips every FLOAT32 point
back to the source within2e-6UU. No shape edit or generated replacement texture.
Original dimensions2.4008 x.6718 x.6707UU. Export scale52.5UU/m, multiplied at
draw by the existing `(52.5/MetresToUU())*(GunScalePct/100)` native size factor;
default gun75/world80 gives about27.44 x7.68 x7.66mm, not giant ammunition.

Private outputs (ignored, no extracted assets in Git):

- `centurion_shell.svrk`: SHA256
  `AB61034E9E35D2C5BEAC937E3E3BB90D15272828FF2BBBE2CD26ED443E5E5CAC`.
- `centurion_shell.svrt`: SHA256
  `F804945B8160EC47C6C5B5C94844C9A8B8CCE43E28499799606051B5909FE8D0`.

Offline extraction (explicit research copy, never installed Steam):

```text
umodel.exe -export -dds -out=<private-export> -path=<offline-package-dir> wp_SP_ARPistol_SF.decompressed.xxx shell_rifle StaticMesh
python tools/reload/prepare_centurion_shells.py --package <offline-package> --mesh <export>/wp_SP_ARPistol_SF/StaticMesh3/shell_rifle.pskx --diffuse <export>/wp_SP_ARPistol_SF/Texture2D/shell_rifle_d.dds --output <private-cache> --preview
```

CPU textured original UV/albedo preview inspected, including axial view. It is
albedo-only, NOT complete UE3 material/shadows or the actual particle simulation.

## Runtime experiment

Default-OFF `[Input] CenturionShellEjection=1`, alongside
`CenturionCylinderAnimation=1` and current manual cylinder gesture flags.
CPU assets load at existing XR initialization, GPU resources warm before a
gesture. Once warm, the new feature skips idle state capture/setup entirely
when no clip/markers/cases need drawing;100,000 idle-policy checks cover this.
OFF retains prior preview scheduling. Missing/invalid assets fail closed. Ejection additionally requires the
actual native cylinder palette/IB-content path to have passed its witness.
That witness is cleared on XR initialization, Reset and palette restore faults.

One `Dumped` event launches at most six original cases, cosmetic spent count =
capacity minus observed remaining clip (capacity must be6). No inventory writes
and no repeated spawn per render/eye/held button. Launch position is the EXISTING
authored manual gesture socket, NOT a newly claimed engine socket. Fixed six-case
storage; analytic650ms flight/spin; no per-frame allocation, I/O or new engine
hook. World transform reuses `TrackingDeltaToWorldUU`, live camera origin,
recenter origin/yaw, base yaw and scale, with a tested inverse and orientation
transport. Thus a case does not follow later gun movement or stick/snap movement
in the mathematical model. Correct native scene alignment still needs L3/L4.
The launch velocities/lifetime are authored cosmetic values, NOT alleged
reverse-engineered native particle distribution constants.

Pause/context/tracking/ammo freshness loss, pawn/weapon change and cancellation
clear the burst. Reset increments presentation generation, so an old published
snapshot cannot resurrect cases. No continuation across menus/death/loads.
Original mesh is submitted separately to each eye, using existing private depth
and a single scoped state capture/depth clear for all instances, maximum12 draws
and1200 submitted triangles per burst frame. Stream/index/state/RT/viewport
restore, fault latch and lifecycle are retained; knife/cylinder's one-instance
entry delegates to the same route without changing its parameters.

Deliberate open work: NO native floor/wall/hand occlusion, collision/bounce,
impact sounds, native particle class/ABI or full scene lighting. Cases disappear
after650ms instead of accumulating. No floor plane is guessed. Native post-request
reload animation remains unchanged and may need later synchronization.

## Shoulder audit and MAIN test

Reload source now uses valid left/right eye MIDPOINT, not left-eye g_headPosXR.
Invalid/missing eye positions refuse a gesture. `ShoulderLeftMetres` is the
actual INI-reader helper: signed legacy22/-22 -> positive.22m, clamp.15..40m.
Pouch local coordinate is `(-left,-down,+back)` rotated about OpenXR +Y by the
actual `DecomposeOpenXrYXZ` head yaw; no walk sign, game yaw, handedness swap or
recenter yaw enters that physical anchor. Yaw is latched at reload start; centre
translation remains head-relative. Target and stock preview use the SAME centre
and function. Existing hip/knife/weapon anchor transforms are not changed.

L1:27,075 independent producer/basis checks (all361 yaw degrees, pitch -80..80,
roll -120..120 and signed/clamped settings);9 IPD/seated-height fixtures, invalid
eye refusal, no right-shoulder acquisition and preview/grab identity. No claim
of real torso tracking: if Circle is pressed while looking90deg sideways, the
estimated torso frame starts sideways too. Begin facing forward relative to
your body; later looking over the shoulder does not rotate the target.
Bounded16 log rows report mid-eye, latchedYaw, local XYZ and actual grab target.
Expected local=(-.22,-.12,+.18)m at default settings.

MAIN integrates on CURRENT source, preserves accepted guard/input/Polish/glove/
knife/mirror INI/assets, and rebuilds; do not deploy this older standalone SIDE
base. Use the existing safe deployment/accepted profile/backup/hash/exit gates.
Add only the scoped flag above, `ReloadShoulderLeftCm=22`, and retain the reviewed
16cm insertion tolerance. Private staging accepts `-ShellAssets <cache>` plus
the prior `-AnimationCache <SVCA>` / `-OriginalAssets <verified clip pair>`;
fixed output `build/centurion-reload/ejection-candidate/`, NO INI staged.

Three-sentence headset script, no mid-test chat:
Face forward, empty the Centurion and perform Circle -> sideways open -> muzzle
down; observe separate textured cases falling in both eyes, with no free refill.
Take the spare with LEFT grip BEHIND LEFT SHOULDER, insert/release near the gun,
flick close and fire, then physically turn90deg and repeat from that new facing.
Check that looking back after Circle doesn't move the supply, right shoulder/
left hip doesn't grab it, pause/resume is clean, then exit normally and report.

## Verification before clean build

L1_PASS:16 CPU executables including all prior reload/knife/input/lifecycle/ABI
regressions;40 Python tests including malformed conversion/reference graph.
Shell sampler verified649 times across676 reference frames plus world inverse,
orientation transport, once/event, identity/cancel/expiry and unchanged counts.
The own-process D3D9Ex fixture rendered actual original case texture pixels in
BOTH eyes (110/114 pixels, centroids138.85/115.12), repeated after Reset, and
verified UP-cleared stream/index buffers, RT/depth/render-state/viewport restore,
invalid count and injected partial draw failure. This fixture loads the SYSTEM
D3D9 only, never the game/proxy/Steam. It is not OpenXR or native game pixels.
Separate earlier original IB upload/read fixture also passes.

L2 clean receipt follows. L3 Steam / native scene alignment / L4 PSVR2 UNKNOWN.
