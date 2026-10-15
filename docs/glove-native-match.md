# PR #30 follow-up — native glove/wrist cut and scale

User feedback: new gloves function, but must look and be sized like native hands
holding the gun, including the short visible wrist/cuff, not gloves cut off alone.
This reopens appearance/size acceptance; it does not accept all of PR #30.

## Proven source changes

- Accepted native face filter moved without changing a range into
  `glove_isolate.h`: 48 inclusive runs, **2918** faces, not the old log's 2922.
- The baker requires an offline decompressed RvGame and matching PSK. All 16512
  cooked indices exactly equal ActorX after the Y-reflection winding swap 0/1.
  It uses that face set and original per-corner UVs, not a new geometric crop.
- Left 922 vertices / 1506 faces = 1482 native +24 cap; right 904 / 1464 =
  1436 native +28 cap. This is the same source surface as the native cut,
  plus closing geometry. Cuff cap follows the actual blended rim at both morph
  endpoints. Zero open welded edges; CPU axial ray hits cap first at 15 cm.
- Original `inview_glove_d.png` -> SVRT BGRA bytes are checked pixel-for-pixel.
  Texture SHA256 B54B80280B590E0FE5C3DA456D1E47C775CDEBE53C85016742A6A2DB4183D4A9.
  Existing original 90-bone noTMD reference/weights author the finger endpoint.
- Runtime mesh offsets use `bakedUnits / MetresToUU() * GunScalePct/100 *
  nativeLocalScale`. That is the same size chain as BuildGunC and
  BuildLiveControllerC, not an independent hand-size knob. The actual native
  uniform LocalToWorld scale is read at the exact audited scene/foreground draw.
  Unknown/nonuniform/sheared state keeps native hands. Wrist targets, IPD,
  head/weapon pose and gun size settings are not written or rescaled.
- With defaults (52.5 baked UU/m, 52.5 world UU/m, 75% mesh, unit local)
  the mesh offsets are 75% of the first candidate, which omitted this factor.
  Both endpoints use the same factor, including finger motion and cap.
- SVRG v2 adds baked units to its header. **Old v1 caches are rejected** rather
  than silently using unknown units; MAIN must update both mesh caches with DLL.
  Original SVRT diffuse is unchanged. No game-derived assets are in Git.
- Removed custom `.55+.45*abs(normal.y)` brightness: source albedo is no longer
  recoloured according to an invented world-up light.

## Explicit unresolved limit

The new renderer remains a **diffuse-only** fixed-function GLB renderer, not
native UE3 Mat_CharacterMat normal/specular/scene lighting. Matching source art
and removing the fake tint does NOT prove pixel-identical native shading.
Full visual equality stays open until MAIN identifies the live native material
and lighting path or provides a validated material bridge. Do not guess sampler
slots/constants or silently change accepted native hands to match the experiment.
No new shader/engine writes or material hooks were added in this correction.

## L1 and L2

L1_PASS: 284 CPU checks, 9 Python geometry/format/native-cut/albedo tests,
fake-D3D guard/refcounts, native glove/cooked-palette/hand-anchor regressions.
Synthetic CPU transforms: 0 allocations, 3600 frames x 2 eyes per hand,
0.0128/0.0131 ms per pair; eager CPU loading 2.181 ms. Not GPU or runtime timings.
Production build has succeeded x86 PE32 /O2; clean-source receipt follows.

New private prepared pack: `build/gloves/native-match/`; candidate will be
`build/gloves/native-match-candidate/`. Old `build/gloves/candidate/` is the
previous snapshot and is NOT the new pack. Never infer which one is installed.

```powershell
python tools/gloves/prepare_gloves.py --gloves <old-rigid-GLB-folder> --psk <offline>/ch_inview_noTMD.psk --cooked <offline>/RvGame.decompressed.xxx --texture <offline>/inview_glove_d.png --output build/gloves/native-match
python tools/gloves/preview_gloves.py --prepared build/gloves/native-match --source <old-rigid-GLB-folder>
tools/gloves/test-gloves.ps1 -PreparedDirectory <absolute>/build/gloves/native-match -SourceTexture <offline>/inview_glove_d.png
spikes/view_matrix/build.ps1 -GameBinaries <absolute>/build/never-deploy
```

MAIN: integrate on the current combined branch, not a stale base-main DLL that
would remove other work. Use current verified backup/hash/lease tooling, update
the DLL plus all three local cache files and enable only GloveRendererEnabled.
One no-chat-switch test: compare empty-hand glove/wrist outline/size, pick up
the pistol and compare native hand size/material at similar distance and light,
then holster and close/open fists. Log `native-size gun=... worldUUperM=...
local=...` and hashes. Size, texture perception and full lighting are separate
verdicts; this is not a whole-feature PHYSICALLY_VERIFIED claim. SIDE did not
read/launch/deploy/restore any Steam file.
