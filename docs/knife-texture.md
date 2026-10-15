# Knife texture correction, PR #31

Observed: `knife.svrk` v1 already contains original `2010_knife` geometry, but
the renderer receives only position + two flat vertex colours. It cannot sample
the original `Mat_Knife` diffuse because it discards the source wedge UVs and
binds no texture. This is not evidence of a custom replacement model.

Hypothesis: preserving the original per-wedge UVs, baking original `knife_d`
without recolouring, and binding it in the scoped overlay draw will remove the
two-colour placeholder appearance. Falsifier: UV layout is wrong/inverted,
the same grey/black regions remain, either eye differs, Reset loses the texture,
or texture upload/rendering disturbs the game. Do not change knife scale/mount,
hip interaction, native attack request or accepted game systems in this patch.

This is a **diffuse/UV correction**, not a claim of full native UE3 material
equivalence. `Mat_Knife` also references normal/specular/reflection maps and
`base_specialWeapon` lighting. Those are not reproduced by fixed-function
albedo rendering. No invented normal-Y tint or guessed UE3 sampler registers.
The original exported normal/specular remain available for a separately proven
material bridge; source references alone do not prove runtime shading.

SIDE only: L1/L2, original assets private, no Steam/deployment/merge.

## Implemented

- SVRK **v2**: same 662 vertices, 1010 triangles, source scale/mount unchanged;
  original per-wedge UVs retained, every vertex white. Old v1 is rejected rather
  than guessing missing UVs. Both new mesh and texture must accompany the DLL.
- `knife.svrt`: original `knife_d` RGBA -> BGRA byte copy, no tint/filter/upscale
  or V flip. Original alpha is retained in the cache but is not treated as
  transparency in the opaque draw; diffuse alpha is not proved an opacity map.
- Export from the private asset library including `Textures_P3.tfc` gives the
  original 256×256 diffuse and normal, 128×128 specular. Earlier lab exports
  without that TFC only supplied 64×64 tail mips. Direct `knife_d` export PASS;
  recursive material export fails later on unrelated `tmd_ring_fx` bulk data,
  and is not counted as a full successful material export.
- CPU eager load mesh+texture, cached DEFAULT texture, temporary SYSTEMMEM
  staging, `UpdateTexture`; no MANAGED allocation or DEFAULT `LockRect`. Reset
  releases GPU texture/state/depth, CPU cache survives. No per-frame I/O or
  new hooks. Captured ALL block restores texture/sampler state; raw texture
  setter avoids corrupting native CPU shader/resource classification.
- Original diffuse replaces the two flat colours in the two existing eye draws.
  Native attack/damage request, hip/grip, scale, world/shadows and hands unchanged.

L1_PASS: 147 interaction checks, seven Python parser/scale/native-UV/exact-BGRA
tests, CPU fake state/refcount restoration, texture row pitch and SYSTEMMEM→
DEFAULT upload success + six failure paths (both creates, lock, unlock, pitch,
update), malformed texture/UV/v1 rejection, existing holster/hand-anchor tests.
CPU eager mesh+texture load 0.413 ms (synthetic, not GPU/no-hitch proof).
Inspected `build/knife/textured/uv-preview.png`: original UV rasterization front
and back, visible blade/rivets/handle detail instead of flat colour blocks.
This CPU preview is **not** a Steam pixel or PSVR2 visual pass.

Private assets and reproducible preparation:

```powershell
python tools/knife/prepare_knife.py --mesh <offline>/2010_knife.pskx --diffuse <offline>/knife_d.tga --output build/knife/textured
python tools/knife/preview_knife.py build/knife/textured
tools/knife/test-knife.ps1 -PreparedMesh <absolute>/build/knife/textured/knife.svrk
spikes/view_matrix/build.ps1 -GameBinaries <absolute>/build/never-deploy
```

Source mesh SHA256 `DA9303A792361EB735124E620F147E490F26EF5A49D4E6C2E897F0B337F5FB5D`.
Diffuse TGA SHA256 `8A7705671A84B7B39943836E3057A2BA78D5DDf7E385326A3AD8D7E556841C56`.
New SVRK SHA256 `F7165692E4F74E69364A6BACD8944515F77837033044A771A048D82B666F1913`.
New SVRT SHA256 `D4686DE723CD4550C6387D274A08A3C88AFFB8C6B3F039165F6E6BCAAA4494F2`.

MAIN must integrate this PR into current combined source and rebuild, not
overwrite newer accepted code with this old-base standalone DLL. Stage **both**
new files at `Binaries/vr-assets/knife/knife.svrk` and `knife.svrt`, enable only
existing default-OFF `HipKnifeEnabled` using verified backup/hash/lease tooling.
One three-sentence no-chat physical test: Before TMD, pick up the hip knife and
rotate it to inspect original blade/rivet/handle detail from both sides and eyes.
Sheathe/reacquire, make a slow motion and one deliberate native melee swing;
verify scale, grip and attack behavior are unchanged.
Pause/resume, exit normally and report texture, stereo/FPS and Reset regressions
separately; normal/specular/native-lighting equivalence is explicitly not claimed.
