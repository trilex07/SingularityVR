# Shadow + SFX handoff — 2026-10-08

This work continues from the exported session `SingularityVR/naprawa-menu-vr-w-grze-singularity.json` and the production branch `fix/vr-menu-input-grip-performance` at `512ffd8`. Existing agent edits were preserved.

## Shadow evidence

- `SHADOW TEST = DROP MAP PASSES` removed roughly 800–840 classified shadow-layout draws per window while the ceiling/wall pattern remained. The pattern therefore is not proven to come from the ordinary shadow-map generation pass.
- The live `ShadowMode=3` census reports the depth shader reading `c0,c5–c14`, but the camera-slot mask stays `0x0`; only `c0` is tracked as a camera slot and is explicitly exempt. The mode-3 unit test is valid mathematically, but the game-side receiver mapping is still unproven.
- The safe baseline remains `VrShadows=0`: first-person caster suppression and projected-shadow neutralisation stay active. A test that changes this and `ShadowMode` together is not a valid comparison.
- Dishonored/KHARVOX references do not provide a stereo shadow-map implementation for this UE3 path. Do not remap light-space matrices by guessed register numbers.

## SFX change in this handoff

`vfx/vfx_atlas_io.h` now treats the muzzle and both smoke atlases as required and the blood atlas as optional. If `vfx_blood.dds` is missing or invalid, a procedural blood fallback is created; muzzle flash and smoke no longer disappear because the unrelated hit atlas failed to load. The ABI and clip IDs are unchanged.

The existing plugin remains fire-driven (flash + two smoke puffs). Blood still waits for a proven hit-point/material seam; no hit-result offsets were guessed.

## Candidate artifact (not deployed)

Directory: `C:\Users\Trilex\vr-mods\deployment\shadow-sfx-clean-20261008`

- `d3d9.dll` — x86 PE32, SHA-256 `BA5826566B00C0943A73F08FC1B8FBB16EEAB73A6CA0F4721498C15207D81489`
- `SingularityVfx.dll` — SHA-256 `2F2A6D23F2EA9B13E4C335571E5B8B7A1C9FA04EF2ACC623B40CB062C8B3C363`
- `SingularityVR.ini` — one `[Render]` section, `ShadowMode=3`, `VrShadows=0`, `VfxDiag=0`, `VfxDraw=2`, `DropExtraTarget=0`

This folder is a side candidate only. It has not been copied into the Steam game directory and does not overwrite the main agent's deployment.

## Desktop validation

- shadow-policy tests: PASS
- ShadowMode=3 policy + light-space compensation invariants: PASS
- VFX batch tests: PASS
- VFX world-draw/state-restore tests: PASS
- production source build: PASS with `/LD /EHsc /W3 /Zi /MD /std:c++17` and x86 toolchain; only existing narrowing warnings in diagnostic cyan-dot initialisers
- atlas-loader compile smoke test: PASS

## Required headset comparison

Use the candidate only after preserving the current Steam DLL. Compare the same room and weapon in this order:

1. `ShadowMode=3`, `VrShadows=0`, `DropExtraTarget=0`, `VfxDiag=0`.
2. Without changing anything else, toggle `SCREEN FX TEST`/`DropExtraTarget=1`.
3. Restore `DropExtraTarget=0`; test the same scene with the game's **High Quality Decals = OFF** as the single game-side variable.
4. Fire once with `VfxLook=SOFT MIST`, then once with `NORMAL`; note whether the flash and smoke are at the muzzle and whether the blood fallback appears only on a proven hit seam.

Record `ShadowMode3`, `VFX host`, `VFX overlay`, and any `VFX dot` lines from `%LOCALAPPDATA%\SingularityVR\view_matrix.log`. Do not use the diagnostic VFX dot in normal play; it is explicitly disabled in this candidate.
