# Singularity VR asset library

This directory has a strict split between source references and runtime data:

- `ui/` contains the downloaded Kenney Input Prompts source archive and its CC0 notice.
- `reference/` contains external rig references used for inspection only. They are not loaded by
  the DLL and do not replace Singularity's native meshes.
- `processed/ui/` contains the small PSVR2 prompt subset and the generated runtime atlas staged by
  `research/native-hands-next/stage-final.py`.
- `processed/reference/` contains extracted metadata and comparison results, not game assets.

The runtime currently uses only `processed/ui/runtime/vr_prompt_atlas.bmp`. It is loaded once and
cached by the menu overlay; no image decoding or file access occurs in the per-frame pointer path.

`reference/vr-hands-rigged/` contains the locally supplied Free VR Hands package. It has no
license/readme file, so it is marked **REFERENCE ONLY — DO NOT DISTRIBUTE** and remains outside
the runtime build. The extracted hierarchy is recorded in
`processed/reference/vr-hands-skeleton.json`.
