# SIDE glove experiment

Base: `origin/main` at `72ad59d`. No Steam access; L1/L2 only.

## Reopened: native appearance and size (2026-10-15)

User reports the gloves work, but requires the **same model appearance and
size as the native gloves holding a gun**. This accepts neither size nor art.
Source evidence: `ApplyMeshScale` / `BuildLiveControllerC` / `BuildGunC`
all use `GunScalePct`; the custom renderer used unscaled baked metres.
`MetresToUU` additionally varies with `WorldScalePct`. The custom colour
factor `.55 + .45*abs(normal.y)` is invented lighting, not UE3's material.

Hypothesis: use the existing native hand's uniform LocalToWorld scale and
the shared GunScalePct/MetresToUU conversion at consumption time, keeping
both wrist positions/IPD untouched. Remove the invented normal-Y colour
factor; pin source skin geometry/UV and original diffuse texels in offline
tests. Version the cache with its actual baked units so custom pack scales
cannot silently bypass the conversion. Falsifier: transformed wrist-relative
native/custom landmark distances differ for a uniform native matrix or live
scale changes; any albedo texel/channel differs from the source PNG.

**Known rendering gap:** the current renderer is diffuse-only, not the native
UE3 normal/specular/scene-light shader. Removing the fake tint is not proof
of identical lighting. Do not call the full appearance requirement fixed
without native material/pass identification and a real side-by-side result.
This update must not silently replace the user's chosen GLB route with a
different hand system or change accepted gun meshes to match a bad glove.

The follow-up request includes the visible wrist/cuff portion after the glove.
Its exact producer is `kNoTmdHandFaces`, now shared in `glove_isolate.h`:
48 inclusive runs / **2918** faces (the old log saying 2922 was inaccurate).
The table is unchanged; native clipping and held-weapon poses are not edited.
The baker proves all **16512** cooked indices against ActorX, swapping corners
0/1 for UEViewer's reflection, then uses exactly those native faces, not an
invented centimetre cut. Original UVs, weights and skeleton are retained.
Left: 1482 original faces +24 cap; right: 1436 original faces +28 cap.
Caps follow the blended cuff rim through the morph, not a frozen disc that
tears away from the native cut. Both endpoints have zero welded boundary edges.

## Producer and hypothesis (before implementation)

`xrLocateSpace` in `PollActions` supplies orientation, decomposed into yaw,
pitch and roll. `BuildLiveControllerC` consumes all three for the native UE3
skinned palette (`PrepareIndependentLeft`). Thus the native wrist is not a
constant-orientation GLB. No GLB loader or glove GLB is tracked on this main.
The separately prepared HUD pack contains rigid, untextured single-mesh GLBs.
The user explicitly chose a **new GLB renderer**, default OFF, after this was
explained. Do not change native gun or hand-palette transforms.

Hypotheses: the extracted open cuff needs actual closing geometry (two-sided
material alone cannot close a hole); named PSK finger joints/weights can author
an open/fist morph without fabricating a skin for an already rigid export.
The target pose is an authored experiment, not physical acceptance.

Falsifiers: ray through the forearm axis still misses the capped surface;
roll/pitch rotation does not rotate the rendered glove; squeeze=1 does not
form a fist or release does not restore open; held gun/aiming changes.

Plan: deterministic local asset preparation from the existing GLBs and PSK,
closed GLB morph + versioned CPU cache; one-time load outside gameplay; no
mesh swaps, per-frame file access or allocations. A narrow native noTMD
free-hand replacement only, with a grip-pose quaternion and independent
analog action; no held-weapon/TMD replacement. Preserve render state, RT,
viewport and stream bindings; release GPU resources for Reset.

## Implementation and limits

`GloveRendererEnabled=0` in `[Render]` is the code and shipped default, read
at XR input startup (restart required). With it OFF, no private pack load, new
XR actions, GPU allocation, native draw suppression or custom draw occurs.
With it ON, a separate `glove_grip_pose` action uses **only** `/grip/pose`,
not the existing mixed aim/grip action; all quaternion components are consumed
directly, without Euler decomposition. A separate float `glove_squeeze` action
uses the existing profile mapper: `squeeze/value`, Index `squeeze/force`.
Physical left/right remain separate in either handedness mode. The deadzone
is 0.08, remapped linearly to [0,1]; inactive/nonfinite input opens the hand.
If suggestions reject the optional bindings, retry without them and disable
the experiment, retaining original bindings/order rather than losing controls.

The existing GLB export is untextured, with POSITION only. The offline tool
checks its vertices against the private, matching `ch_inview_noTMD.psk`, uses
that asset's named skeleton/weights to author the fist endpoint, and recovers
UVs from its wedges (raw Direct3D V, no OBJ-style flip; the
[glTF specification](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html)
also defines image origin at the upper left). It writes a one-primitive **GLB with one morph target**,
normals, UV and embedded native glove diffuse texture, plus SVRG/SVRT caches.
The output GLB still has no skeleton: its morph is already baked from the
90-bone source. No copyrighted mesh or texture is committed to Git.

The D3D9 renderer consumes this narrow **offline-baked GLB cache**, not an
arbitrary general-purpose GLB scene. SVRG v2 has a 24-byte header including
baked mesh-units/metre and reserved-zero flags, then
14 float32 values per vertex (open/delta/normal/normalDelta/UV) and uint32
triangle indices; SVRT v1 has dimensions and BGRA8 texels. Lengths, counts,
indices and float ranges are checked before accepting the pair. Both morph
endpoints remain resident; grip does not swap or reload meshes.

This is deliberately the **free-hand/noTMD** vertical slice: both poses must
be tracked, gameplay stereo focused, no menu/pause/cinematic, no held weapon
and no support grip. Suppression additionally requires the exact previously
measured noTMD section (3314/2924 vertices, index 0, 4830 triangles), scene
target, non-shadow CTAB layout and foreground window. Rendering waits until
that native section was actually suppressed in the current frame; it cannot
add a second pair on an unrelated mesh. Held gun, authored pistol grip, TMD,
weapon aiming and native shadow caster paths are not rewritten. The new
gloves do not yet replace the free support hand while a weapon is held.

One indexed draw per hand per eye = **four custom draws/frame**, never one
per finger; only MAIN runtime evidence can confirm those calls execute.
The input/morph/CPU-transform path has no per-frame heap or file operations.
GPU resources are initialized once (normally in menus), use DEFAULT pool
with dynamic texture upload, and are released through `ReleaseWristResources`
for Reset. CPU data survives Reset. No additional vtable hooks are installed.

The private hand depth surface handles self-occlusion but intentionally does
not borrow world depth: gloves currently overlay world geometry, matching a
first-person overlay experiment rather than claiming physical collisions.
Only non-MSAA SBS is supported. D3DSBT_ALL plus explicit RT0/MRT/DS/viewport
restore protects GPU state; the original driver callbacks bypass the mod's
shader/RT/texture/clear/viewport observation hooks so their CPU caches are not
corrupted by the overlay. A draw/setup failure disables the experiment and
native hands resume next frame; a suppressed frame cannot retroactively be
redrawn. Real driver failure/reset behavior remains MAIN test debt.

Grip-local axes are +X right, +Y dorsal, -Z fingers, with the native wrist
as origin. Baked units default to 52.5, converted at runtime by
`bakedUnits / MetresToUU() * GunScalePct/100 * nativeLocalScale`.
LocalToWorld uniform scale is read from the exact known draw; unknown/sheared
or nonuniform transforms preserve native hands. There is still
**no measured Sense grip-to-wrist mounting offset yet**. Position,
size, finger self-intersections during linear blending and comfort require
physical validation, not a claim that an authored closed silhouette is final.

## Measurements and offline evidence

- Original left: 30,000 bytes, 817 vertices, 1607 triangles; dimensions
  12.8133 × 14.7207 × 9.71785 mesh units.
- Original right: 30,324 bytes, 826 vertices, 1625 triangles; dimensions
  13.3652 × 14.7480 × 9.71785 mesh units.
- Each original: 1 node, 0 skins, 0 animations, 0 UV attributes,
  single-sided material; private source skeleton has 90 bones.
- Output left: 963 UV-split vertices, 1582 triangles including 25 cap
  triangles; output right: 972 vertices, 1602 triangles including 24 caps.
  Degenerate boundary placeholders in the old extraction are excluded.
- Each source has one open cuff loop; each output has **zero open edges**
  after capping, including at the fist endpoint (UV seams weld geometrically).
- Axial ray from 15 cm behind each cuff: new first hit is the cap at 15 cm;
  original first hit was approximately 35.09/35.97 cm, through the empty cuff.
  `geometry-preview.png` is a CPU-rendered before/after, **not game pixels**.
- Five x86 `/O2` CPU executables: glove math/cache/analog (261 checks), fake
  state guard sequencing/refcounts, existing glove isolation, cooked noTMD
  palette, and hand-anchor matrix regression; plus seven Python geometry/
  format tests, including the private pair. Exact endpoints, deadzone,
  nonfinite inputs, malformed/truncated cache rejection, all rotation axes,
  inverse eye pose, per-eye separation and weapon exclusion are covered.
- 3600-frame analog sweep, two eyes, real 963/972-vertex buffers: **zero
  allocations**, same scratch address; approximately 0.03–0.04 ms per
  hand/two-eye CPU transform on this machine. Eager CPU pair+texture load was
  approximately 2.5 ms in the standalone test. These are not GPU/frame-time
  or physical no-hitch claims; real startup/driver times are separately logged.

## Reproduce without Steam

Python dependencies: numpy and Pillow, already available in the task environment.
Use explicit local source paths (the sibling HUD pack is not merged into main):

```powershell
python tools/gloves/prepare_gloves.py --gloves <existing-GLB-folder> --psk <private-noTMD.psk> --texture <private-inview_glove_d.png> --output build/gloves/prepared
python tools/gloves/preview_gloves.py --prepared build/gloves/prepared --source <existing-GLB-folder>
tools/gloves/test-gloves.ps1 -PreparedDirectory <absolute-prepared-folder>
spikes/view_matrix/build.ps1 -GameBinaries <local-unused-directory>
```

Never pass `-Install`. The explicit local `GameBinaries` avoids even the build
helper's automatic installed-game path discovery. The asset tools write only
to the supplied offline output, never deploy, and reject source/output overlap.
Build-only L2 checks x86 `0x014c`, PE32 `0x010b`, `/O2`, source provenance and
DLL SHA in `spikes/view_matrix/d3d9.build.json`. Stock build warnings remain;
the new standalone CPU tests compile `/W4 /WX`.

The local private output is `build/gloves/prepared/` in this worktree (source
GLB, PSK and texture hashes plus output hashes are in its `report.json`).
The runtime cache inputs are exactly `glove_left.svrg`, `glove_right.svrg`,
`glove.svrt`; generated GLBs are the reference/preview assets, not read by the
DLL. MAIN must put the three cache files beside its candidate under
`vr-assets/hud/katorga/glove-animation/`, using its own backup/hash/deploy
workflow, and temporarily enable `GloveRendererEnabled=1`; never replace
the entire installed INI with the shipped example.

## MAIN test: exactly three sentences, stay in the headset

Rotacja: w bezpiecznym miejscu przed zdobyciem TMD, z pustymi dłońmi, obróć
osobno oba kontrolery w roll/pitch/yaw i sprawdź, czy rękawice podążają bez
skoków i odwrócenia osi.
Dziura: skieruj otwarty mankiet każdej rękawicy w stronę oka i porównaj z
kontrolą OFF, oczekując zamkniętego końca bez prześwitów i podwójnych dłoni.
Pięść: powoli ściśnij i puść grip każdej ręki kilka razy, potem dobądź broń,
sprawdź niezmieniony chwyt/celowanie oraz płynność i zamknij grę bez wracania
do czatu po drodze.

Status: L1_PASS; final clean-source L2 artifact recorded in the companion
evidence note; **L3/L4 pending MAIN**, never PHYSICALLY_VERIFIED by SIDE.
