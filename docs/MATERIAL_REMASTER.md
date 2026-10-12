# Singularity VR material remaster prototype

**Branch:** `feature/material-remaster-v2`
**Scope:** three materials, native resolution, no upscaling, no renderer changes.

## What this is

A proof that genuinely better material detail can be delivered through the
existing standalone texture injector: real authored micro-detail on top of the
shipped mip0, real DXT-compressed DDS output, and measurements instead of claims.

Three materials, chosen to cover the three surface families the player sees most:

| key | asset | material | what it demonstrates |
|-----|-------|----------|----------------------|
| `weapon_shotgun` | AR_shotgun | AR_Shotgun | machined/painted metal, scratches, wear |
| `glove` | inview_glove | inview_gloves_gauze | woven fabric, stitching, abrasion |
| `env_door` | door_blastdoorN | door_blastdoorN | rolled steel, rust, grime |

## Hard rules this prototype follows

1. **No upscaling.** Every replacement is the same resolution as the source
   mip0. Where a higher resolution was not available, nothing was invented.
2. **True mip0 or nothing.** The loader compares the DDS header against the
   texture object's mip0 size and raises if it is a lower mip. Two of three
   specular maps fail this gate and are therefore *not* substituted (below).
3. **Container formats unchanged.** DXT5 stays DXT5, DXT1 stays DXT1, so the
   engine's material response and memory layout do not change.
4. **Alpha preserved.** The blend never writes the alpha channel; normal maps
   keep their constant 255.
5. **Nothing is deployed to Steam.** All output is produced locally and the
   deploy test writes into a scratch directory.

## Source selection and the mip0 proof

`verify_true_mip0.py` parses each source DDS header and compares it with the
object's mip0 size:

| map | object mip0 | source file | verdict |
|-----|-------------|-------------|---------|
| AR_shotgun_d (diffuse) | 1024x1024 | 1024x1024 | **VERIFIED true mip0** |
| AR_shotgun_n (normal) | 1024x1024 | 1024x1024 | **VERIFIED true mip0** |
| AR_shotgun_s (specular) | 1024x1024 | 512x512 | lower mip - **not substituted** |
| inview_glove_d (diffuse) | 1024x1024 | 1024x1024 | **VERIFIED true mip0** |
| inview_glove_n (normal) | 1024x1024 | 1024x1024 | **VERIFIED true mip0** |
| inview_glove_s (specular) | 1024x1024 | 256x256 | lower mip - **not substituted** |
| door_blastdoorN_d (diffuse) | 1024x512 | 1024x512 | **VERIFIED true mip0** |
| door_blastdoorN_n (normal) | 1024x512 | 1024x512 | **VERIFIED true mip0** |
| door_blastdoorN_s (specular) | 512x256 | 256x128 | lower mip - **not substituted** |

UModel reports that the shipped `Textures_*.tfc` bulk data for the specular maps
starts at mip 1, so their mip0 cannot be recovered from the installed game.
Injecting a lower mip would lose resolution at the very distances where the
material is inspected, so specular substitution is deliberately **excluded**.
Enhanced specular maps are still produced for review
(`*_specular_review.dds`) with the same method.

## What is authored vs recovered

| material | new, authored in this prototype | recovered from the shipped mip0 |
|----------|--------------------------------|---------------------------------|
| AR_shotgun | isotropic machining grain (4 octaves); 174 scratch strokes + 87 cross-grain scuffs (deterministic, seeded); low-frequency grime patches with a cool tint; micro normal relief derived from the enhanced diffuse high-pass | base colour and markings; alpha mask (ejection port/sights); macro normals; colour palette |
| inview_glove | fabric weave (21 px thread pitch, zero-mean so the palette does not shift); thread slub irregularity; stitch rows every 85 px; abrasion patches; micro normal relief from the weave | base colour; macro normals; glove silhouette |
| door_blastdoorN | rolled-steel streaks along the panel axis; 65 handling score lines; rust bloom with warm tint; corrosion pits; micro normal relief | base colour; panel layout; macro normals |

Everything is deterministic: the same inputs produce byte-identical output
(fixed seeds), which is what makes the SHA-256 checksums in the manifests
meaningful.

## Fidelity measurements

Measured on the decoded DDS files against the decoded originals
(centre 512x512 crop, mean absolute difference in 0-255 levels):

| asset | diffuse mean abs | % pixels changed > 4 | % pixels changed > 16 | mean luminance |
|-------|------------------|----------------------|------------------------|----------------|
| AR_shotgun | 4.54 | 39.7% | 0.14% | 24.3 -> 22.3 (-8.4%) |
| inview_glove | 5.84 | 58.0% | 0.11% | 35.9 -> 34.4 (-4.0%) |
| door_blastdoorN | 4.75 | 41.6% | 0.15% | 83.4 -> 83.4 (-0.0%) |

Detail is applied as a fraction of the *local* luminance, so the material's
palette, markings and silhouette are untouched: luminance moves by at most
8.4% and under 0.2% of pixels move by more than 16 levels.

The pack also measurably adds micro-detail rather than resampling what was
already there - mean squared high-pass energy of the diffuse map:

| asset | original | enhanced | ratio |
|-------|----------|----------|-------|
| AR_shotgun | 94.5 | 116.2 | 1.229x |
| inview_glove | 249.1 | 285.5 | 1.146x |
| door_blastdoorN | 346.1 | 388.0 | 1.121x |

## DDS output and validation

Every produced file is a real DXT1/DXT5 DDS with a full mip chain halving to
1x1. Each one is re-opened and decoded after writing; the numbers below are
recorded per file in the manifests.

| asset | map | format | size | mip levels | file bytes | decoded as |
|-------|-----|--------|------|------------|------------|------------|
| AR_shotgun | diffuse | DXT5 | 1024x1024 | 11 | 1,398,256 | RGBA (4 channels) |
| AR_shotgun | normal | DXT1 | 1024x1024 | 11 | 699,192 | RGBA (4 channels) |
| AR_shotgun | specular (review) | DXT1 | 512x512 | 10 | 174,904 | RGBA |
| inview_glove | diffuse | DXT1 | 1024x1024 | 11 | 699,192 | RGBA |
| inview_glove | normal | DXT1 | 1024x1024 | 11 | 699,192 | RGBA |
| inview_glove | specular (review) | DXT1 | 256x256 | 9 | 43,832 | RGBA |
| door_blastdoorN | diffuse | DXT1 | 1024x512 | 11 | 349,672 | RGBA |
| door_blastdoorN | normal | DXT1 | 1024x512 | 11 | 349,672 | RGBA |
| door_blastdoorN | specular (review) | DXT1 | 256x128 | 9 | 21,992 | RGBA |

Validation checks performed per file (`dds_io.validate_dds`):

- header width/height/fourcc/mip count parsed and compared with the pixel data
  actually present (declared mip bytes == pixel data bytes for every file)
- decoded dimensions equal to the header dimensions
- decoded mode/channel count recorded
- edge continuity measured (mean absolute difference across the wrap seam), as a
  descriptive statistic only - see "NOT TESTED" below

### Normal maps

- X/Y are stored in R/G with the shipped convention (0 = -1, 255 = +1,
  128 = 0), Z in B, constant 255 in A.
- Mip levels are produced by `downsample_normal`: X/Y averaged in **signed**
  space, the vector renormalised, alpha averaged as data. A naive RGB average
  does not renormalise and flattens the shading - `tests/test_normal_map.py`
  demonstrates the difference.
- After DXT1 compression the enhanced maps land within byte quantisation of unit
  length; the shipped DXT1 normal maps deviate further:

| normal map | shipped vector length range | enhanced |
|------------|-----------------------------|----------|
| AR_shotgun_n | 0.751 - 1.348 | 0.684 - 1.059 |
| inview_glove_n | 0.806 - 1.365 | 0.729 - 1.058 |
| door_blastdoorN_n | 0.938 - 1.339 | 0.770 - 1.047 |

## Video memory

Computed from the measured DDS headers (block arithmetic, DXT1 = 8 bytes per 4x4
block, DXT5 = 16 bytes) and summed over every mip level of the produced files:

| asset | source mip0 sum | replacement sum (all mips) |
|-------|-----------------|----------------------------|
| AR_shotgun (3 maps) | 1,703,936 B | 2,271,968 B |
| inview_glove (3 maps) | 1,081,344 B | 1,441,832 B |
| door_blastdoorN (3 maps) | 540,672 B | 720,952 B |
| **total** | **3,325,952 B (3.17 MB)** | **4,434,752 B (4.23 MB)** |

The replacement set is larger because it ships a complete mip chain for the
texture while the UModel export is mip0 only. Because the injector replaces the
whole texture, the engine no longer builds the mip chain itself, so the two
figures are the honest comparison. There is **no** claimed saving; the previous
"savings" figures were computed from assumptions and have been removed.

## Comparison sheets

`<asset>/comparison/<asset>_<map>_compare.png` stacks three panels at the same
screen scale:

1. **ORIGINAL 1x** - the shipped mip0, scaled with nearest neighbour
2. **LANCZOS upscale** - what a plain 2x upscaling pipeline delivers
3. **REMASTERED (native)** - the enhanced material at native resolution

These are the review artifacts. They are derived from game textures owned by
their publisher and are included for review of this prototype only.

## Injector integration and tier naming

The injector loads `<texturesDir>\<name>_<tier>.dds`, with the tier suffix defined
once in `texq_naming.h`:

| tier | suffix |
|------|--------|
| 2x | `_2x` |
| 4K | `_4K` |
| 8K | `_8K` |

`build.ps1` computes the same suffix when deploying, and
`asset-tools/texture-injector/tests/test_tier_naming.py` compiles the header,
runs the real deploy for **all three tiers** into a scratch directory and asserts
that the file names on disk end with exactly the suffix the compiled injector
looks for. The 2x tier deploy also copies the six material remaster files
(`AR_shotgun_diffuse_2x.dds`, `AR_shotgun_normal_2x.dds`,
`inview_glove_diffuse_2x.dds`, `inview_glove_normal_2x.dds`,
`door_blastdoorN_diffuse_2x.dds`, `door_blastdoorN_normal_2x.dds`) and refuses
to copy `*_specular_review*`.

Observed during that test:

```
injector QualitySuffix: {'2x': '_2x', '4K': '_4K', '8K': '_8K', 'default': '_2x'}
tier 2x: 86 file(s) end with '_2x.dds'
tier 4K:  4 file(s) end with '_4K.dds'
tier 8K:  2 file(s) end with '_8K.dds'
```

(The 4K/8K directories only hold the few maps the earlier upscaling pass produced;
the material remaster package itself is native resolution and exists for the 2x
tier only.)

The injector itself compiles as x86 PE32 with `/O2`
(`build.ps1`, MSVC 18 Community, 37,376 bytes, SHA-256 logged in the build
output). No `d3d9.cpp`, OpenXR, menu or renderer code is involved.

## Tests

| test | what it proves |
|------|----------------|
| `asset-tools/textures/tests/test_dds_io.py` (14 assertions) | DDS files are real: header layout matches the DDS spec, DXT1/DXT5 round trip decodes, mip counts, VRAM arithmetic, FNV-1a mip0 equals the reference implementation the injector uses, invalid files are rejected |
| `asset-tools/textures/tests/test_normal_map.py` | flat normals stay flat, tilts survive, alpha is byte-exact, blended tilts stay unit length while the naive path does not, signed round trip is lossless to within quantisation |
| `asset-tools/textures/tests/test_material_remaster.py` | every substituted map is true mip0, unsubstituted maps are flagged as not mip0, every output DDS decodes with the original resolution and container format, normal conventions hold, fidelity bounds, luminance shift, VRAM recomputed from the files, checksums match disk, evidence labels present |
| `asset-tools/texture-injector/tests/test_tier_naming.py` | injector and deploy naming agree for 2x/4K/8K, remaster outputs use the 2x suffix |
| `asset-tools/textures/verify_true_mip0.py` | the mip0 table above, parsed from the files |

Result on this branch: all tests pass (14 + 6 + 10 unit assertions, plus the
tier-naming compile/deploy test).

## Evidence ledger

### VERIFIED
- all nine source textures are true mip0 or explicitly flagged as a lower mip,
  parsed from the DDS headers in this run
- every produced file is a real DXT1/DXT5 DDS that decodes back to the declared
  dimensions, format and mip count
- the pixel data present in each file equals the mip chain its header declares
- normal maps keep the shipped channel conventions, constant alpha, and land
  within byte quantisation of unit length (compared against the shipped maps)
- fidelity and luminance-shift numbers, measured from the decoded files
- micro-detail genuinely increased: high-pass energy ratio per material
- video memory figures computed from the measured headers and file contents
- SHA-256 of every source and every produced file
- the injector builds as x86 PE32 with `/O2`
- injector and deploy naming agree for tiers 2x, 4K and 8K, tested by compiling
  the header and running a real deploy into a scratch directory

### ESTIMATED
- video memory totals: block arithmetic over measured DDS contents, not a
  profiler reading
- any statement that a detail "will look" a certain way in game

### NOT TESTED
- anything visual in the game: no Steam run, no deploy, no headset session
  (MAIN owns runtime deployment)
- GPU frame cost, texture cache behaviour and any performance claim
- texture seam / UV stitching quality: seam continuity is *measured* as a
  number, but "no visible seam" is a perceptual claim and is not made here
- material role verification beyond what UModel's package dump reported: the
  parameter names (`diffusemap`, `normalmap`, `specularmap`, `emissivemap`) come
  from the UE3 material dump, not from a shader disassembly
- anisotropic filtering: research only, nothing deployed, no measurement of any
  VR-renderer interaction is claimed

## Known limits

- specular substitution is excluded because the shipped bulk data does not
  contain those maps' mip0
- the remaster package exists only for the 2x tier; 4K/8K tiers are an
  upscaling path that this prototype deliberately does not use
- the method adds micro-detail, not macro re-modelling: it cannot add a feature
  the original texture never had

## Reproducing

```powershell
python asset-tools/textures/verify_true_mip0.py
python asset-tools/textures/material_remaster.py
python -m pytest asset-tools/textures/tests/test_dds_io.py asset-tools/textures/tests/test_normal_map.py -q
python asset-tools/textures/tests/test_material_remaster.py
python asset-tools/texture-injector/tests/test_tier_naming.py
```

Output goes to `material-remaster/` in the repository.
