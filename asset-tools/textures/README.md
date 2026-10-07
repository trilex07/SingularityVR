# `asset-tools/textures` — first-person texture upscale pipeline (research)

Reproducible pipeline for a **2× linear** high-res texture experiment covering
only the first-person TMD hands/arms and first-person player weapons of
*Singularity (2010)*.

> Status: **research / offline processing only.** It does not modify the game
> installation, `CookedPC` packages, the production worktree, or the VR DLL, and
> it does not install anything into the game.

## Hard rules

1. **Never commit game pixels.** Extracted `.dds` / `.png` / `.tga` / `.tfc`
   files and every upscaled derivative live under `--local-root`
   (default `C:\Users\Trilex\vr-mods\SingularityVR-assets-local`) and are
   git-ignored. Only scripts, JSON manifests, hashes and docs are tracked.
2. **Never write to the game.** UModel is always pointed at a scratch search dir
   that contains *hard-links* to the read-only packages plus aliases of
   `coalesced.tfc`. The originals are untouched.
3. **Never guess map types.** Classification comes from the authoritative
   material `ParameterName` (`diffusemap`, `normalmap`, `specularmap`, …), not
   from the file name. Anything without a known parameter stays `UNKNOWN`.

## Why UModel needs a TFC alias

Singularity ships a single `RvGame\CookedPC\coalesced.tfc`, but the packages
reference texture caches named `Textures_P0..P3`. `ensure_scratch()` hard-links
`coalesced.tfc` to `Textures_P0.tfc … Textures_P3.tfc` in the scratch dir. See
the Gildor forum thread on Singularity for the original community workaround.

**Important limitation found here:** the coalesced cache only holds a subset of
each texture's mip chain. UModel exports *the highest-resolution mip that is
actually present* — which is frequently **not** mip 0. The pipeline records
`exported_dims` / `exported_mip_level` for every texture and the validator warns
when a candidate was built from mip > 0. Textures whose higher mips are absent
or whose lower-mip read aborts are reported as `dds=False`.

## Usage

```powershell
python -m pip install -r asset-tools/textures/requirements.txt

# full first experiment: build -> classify -> upscale -> validate -> vram
python asset-tools/textures/texture_pipeline.py experiment

# or step by step
python asset-tools/textures/texture_pipeline.py build      # UModel dump+export -> manifests
python asset-tools/textures/texture_pipeline.py classify   # role classes
python asset-tools/textures/texture_pipeline.py upscale    # 2x candidates A/B + mips + crops
python asset-tools/textures/texture_pipeline.py validate   # original vs candidate metadata
python asset-tools/textures/texture_pipeline.py vram       # GPU memory budget
```

Paths can be overridden globally: `--game`, `--local-root`, `--umodel`.

`extract_fp_textures.ps1` is a thin PowerShell wrapper for the `build` step.

## Pipeline stages

| stage | file / action | output (tracked) |
| --- | --- | --- |
| extract | UModel `-dump` / `-export` against scratch hard-links | local `original/`, `original-png/`, `logs/` |
| metadata | parse `-dump` text | `manifests/texture_metadata.json` |
| material map | parse mesh + material dumps | `manifests/material_map.json` |
| classify | `ParameterName` → class | `manifests/classification.json` |
| upscale | Lanczos **A**, iterative back-projection **B**, mip chain, DXT re-encode | local `upscaled/` |
| validate | dims/format/mips/alpha/hash diffs | `manifests/validation.json` |
| budget | DXT bytes × mip chain | `manifests/vram_budget.json` |

## Upscale methods (conservative, non-generative)

* **A — Lanczos-4 resample.** Pure windowed-sinc interpolation. Never invents
  content; safest for text, stencils and markings.
* **B — Iterative back-projection (IBP).** Starts from A and iteratively refines
  the high-res estimate so that its down-sample matches the source. Recovers
  edge detail that is *consistent with the original*; no generative model.
* **Normal maps** are never run through a photographic upscaler. Channels are
  interpolated, then vectors are renormalised per level and re-encoded as the
  original RGB tangent-space normal.
* **Specular / masks / gloss** use plain Lanczos (or bicubic), never
  generative SR. A restrained, threshold-limited unsharp helper exists for
  optional cleanup.

Generative SR (Real-ESRGAN / waifu2x) is deliberately **not** part of the first
experiment because of the hallucinated-text risk on weapon markings; see the
plan for the review gate before it can be added.

## Mip chain / DDS

`save_dds_mips()` writes a proper multi-mip DDS by DXT-encoding each level with
Pillow and assembling a correct header. Normal levels are renormalised before
encoding. For production quality, swap the encoder for Microsoft `texconv`
(see the plan); the interface is a single function.
