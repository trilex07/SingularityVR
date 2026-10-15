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
