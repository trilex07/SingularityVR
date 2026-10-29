# Kasimov SNV-E99 private HD and weighted animation handoff

Local package: `C:/Users/Trilex/vr-mods/kasimov_snv_e99/final_handoff/`.
Permitted source scripts are committed; game-derived files stay outside Git.

The package contains `kasimov_hd_animated.blend`, `kasimov_hd_animated.glb`,
`kasimov_hd_magazine.glb`, 4096x2048 PBR maps, the original PSK and validated
239-frame/60-Hz SVWA, source data, skin/socket manifests, controlled-lighting
original/HD renders, an animated inspection render, and complete SHA-256 hashes.

Original topology: 9,910 PSK points, 11,389 wedges and 13,110 triangles, including
168 separately materialized scope/lens triangles. All 80 original bone names
and parent links are preserved. All 143 dual-influence source points retain
their original named RAWWEIGHTS through UV wedge duplication. No dominant-bone
approximation is used. Blender rest axes are adapted to its bone representation;
original bind matrices are retained in the skin manifest and the resulting
deformation is verified against original inverse-bind linear blend skinning.

CPU-versus-Blender maximum error is 0.000001008 m; exported-GLB reimport versus
saved-scene maximum error is 0.000001137 m. Tolerance is 0.0001 m, measured at
frames 0,30,80,120,180,238. Reimport also checks original named influences at
matching rest positions, normalization, and bone parent equality. The GLB has
JOINTS_0/WEIGHTS_0, 80 inverse binds, original inherited wire motion and optional
modular source-reference actions. `cog_sniper` is never authored with root motion.

Thirteen restrained chamfered machined details are anchored to measured source
triangle surfaces and follow their original body, magazine or wire bone. Existing
private remastered 4K DDS inputs supply surface artwork; metallic/roughness are
conservative authored PBR estimates. Native normal G is converted and normalized
for Blender/glTF. Original/HD albedo comparisons use identical camera and lighting.
The lens retains its original separate geometry and an opaque reflective offline
material. This material is a reconstruction, not proof of the original UE3 shader.
No alpha blending is required by this package.

The detached magazine GLB contains evaluated magazine geometry and its rigid HD
details without skin or animation dependencies, in original clip-local space.
MAIN owns physical magazine movement, release events, native ammunition and
attachment. Source magazine paths are reference actions; they must not override
controller ownership. Wire actions introduce no mandatory manual bolt gesture.

Reproduce from this checkout:

1. `python tools/assets/prepare_kasimov.py <final_handoff>`
2. `blender --background --factory-startup --python tools/assets/blender_kasimov.py -- <final_handoff>`
3. `blender --background --factory-startup --python tools/assets/validate_kasimov_roundtrip.py -- <final_handoff>`
4. `python tools/assets/package_kasimov.py <final_handoff>`

Blender used: official portable 5.2.2 LTS already validated for Volk. The scripts
reuse the existing PSK decoder, SVWA key math and Blender material/render helpers.
The source albedo cache is a lower-resolution original reference; the 4K DDS is
remastered artwork, not evidence of a newly recovered native mip0.

Package validation: OFFLINE_BLENDER_PASS for measured skin, animation and export
gates. Native texture identities are `ar_sniper_rifle_d/n`; this PBR/geometry
package needs MAIN's native material/geometry integration. No Steam, runtime DLL,
INI or MAIN worktree changes were performed. Physical PSVR2 acceptance: NOT RUN.
