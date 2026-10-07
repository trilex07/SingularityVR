# Singularity VR — First-Person High-Res Texture Upscale Plan

Research-only investigation of a **2× linear** texture upgrade for the *first-person
TMD hands/arms* and *first-person player weapons* of Singularity (2010). **Nothing
is installed into the game.** No packages, executable, production worktree or VR
DLL were modified.

* Production HEAD inspected read-only: `71fa0361ffd9eef6bc978707c205de2c31af2f9a`
  (`fix/vr-menu-input-grip-performance`).
* Research worktree: `C:\Users\Trilex\vr-mods\SingularityVR-assets`
  (`research/texture-upscale`, based on the production HEAD above).
* Local-only asset root (git-ignored): `C:\Users\Trilex\vr-mods\SingularityVR-assets-local`
* Pipeline: `asset-tools/textures/` (Python 3.14 + Pillow + numpy, UModel 1590).
* Manifests (tracked): `research/texture-upscale/manifests/*.json`

Game pixels are never committed. Only scripts, JSON (metadata/hashes/filenames),
and this document are tracked.

---

## HAND / TMD ASSETS

First-person arms carry the **TMD** device. Two meshes share the same two
materials:

| mesh | package | material slots |
| --- | --- | --- |
| `ch_inview_tmd_mesh` | `RvGame.xxx` | `inview_gloves_gauze` ×1, `tmd_base_optimized` ×2 |
| `ch_inview_noTMD`     | `RvGame.xxx` | `inview_gloves_gauze` ×1, `tmd_base_optimized` ×1 |

**`ch_inview_arms.Materials.inview_gloves_gauze`** — the gloved hands (highest
priority: permanently on screen, centimetres from the eyes in VR):

| param | texture | class | object res | format | cache |
| --- | --- | --- | --- | --- | --- |
| `diffusemap` | `inview_glove_d` | ALBEDO | 1024×1024 | DXT1/BC1 | P3 |
| `normalmap` | `inview_glove_n` | NORMAL | 1024×1024 | DXT1/BC1 | P3 |
| `specularmap` | `inview_glove_s` | SPECULAR | 1024×1024 | DXT1/BC1 | P3 |
| `detailmap` | `DETAILfinepitted02a_n` | DETAIL (normal) | 256×256 | DXT1/BC1 | P3 |
| `SphericalMap` | `matballInview` | SPHERICAL | 256×256 | DXT1/BC1 | P3 |

**`wp_tmd.Materials.Base.tmd_base_optimized`** — the TMD device body:

| param | texture | class | object res | format | cache |
| --- | --- | --- | --- | --- | --- |
| `diffusemap` | `TMD_new2_d` | ALBEDO | 2048×1024 | DXT5/BC3 | P3 |
| `normalmap` | `TMD_new_n` | NORMAL | 2048×1024 | DXT1/BC1 | P3 |
| `specularmap` | `TMD_new2_s` | SPECULAR | 512×256 | DXT1/BC1 | P3 |
| `emissivemask` | `TMD_new_s` | EMISSIVE | 2048×1024 | DXT1/BC1 | P3 |
| `reflectionmap` | `CM_BrightReflection_01` | REFLECTION (cubemap) | — | — | — |

Hands, gloves and sleeve are **one shared gloves texture**; there is **no separate
hand / glove / sleeve / device texture split** — the glove atlas covers them all.
The TMD device has its own set. The gas-mask / radio / journal are *skeletal
sockets* on the same mesh, not separate textures.

## WEAPON ASSETS

First-person player weapons (the mesh actually seen at the camera; world/NPC
variants are out of scope):

| weapon | FP mesh | package | diffuse | normal | specular | other | max res |
| --- | --- | --- | --- | --- | --- | --- | --- |
| E99 assault rifle | `ch_inview_machinegun_mesh` | RvGame | `AR_E99Rifle_alt_d` | `AR_E99Rifle_n` | `AR_E99Rifle_alt_s` | `AR_E99Rifle_alt_e` (emissive), `AR_E99RifleLens_m` (scope mask) | 2048×1024 |
| Pistol (+laser) | `ch_inview_pistol_mesh` | RvGameMPLoader_SF | `handgun_d` | `handgun_n` | `handgun_s` (also env mask) | `handgun_g` (emissive), `laser_d/n/s` | 2048×2048 |
| Shotgun | `ch_inview_shotgun_mesh` | RvGameMPLoader_SF | `AR_shotgun_d` | `AR_shotgun_n` | `AR_shotgun_s` | `AR_shotgun_rim` (rim mask), `AR_shotgun_e` (emissive) | 1024×1024 |
| Sniper rifle | `ch_inview_sniper_mesh` | RvGameMPLoader_SF | `ar_sniper_rifle_d` | `ar_sniper_rifle_n` | `ar_sniper_rifle_s` | lens uses 32×32 `grey` | 2048×1024 |
| Gatling | `ch_inview_gatling_mesh` | RvGameMPLoader_SF | `ar_gatling_gun_d` | `ar_gatling_gun_n` | `ar_gatling_gun_s` | `ar_gatling_ammo_chain_d/n/s` (belt) | 2048×1024 |
| Railgun / beam | `ch_inview_railgun_mesh` | RvGameMPLoader_SF | `elite_beam_d`, `elite_beam_scope_d` | `elite_beam_n`, `elite_beam_scope_n` | `elite_beam_s`, `elite_beam_scope_s` | `scope_glass_d/o`, `reddot`, `elite_beam_ammo_*` | 2048×2048 |

`ch_inview_alt_pistol_mesh` and `ch_inview_MP_mesh` exist in the packages but were
not resolved from the probed packages (likely multiplayer-only variants); they
are **not** part of the single-player first-person set and are out of scope.

## MATERIAL MAPS

Classification is taken from the authoritative `ParameterName` in each
`MaterialInstanceConstant`, never guessed from the filename. Full data:
`manifests/material_map.json`, `manifests/classification.json`.

| class | count | examples |
| --- | --- | --- |
| ALBEDO | 14 | `inview_glove_d`, `AR_E99Rifle_alt_d`, `AR_shotgun_d`, `handgun_d` |
| NORMAL | 12 | `inview_glove_n`, `TMD_new_n`, `AR_E99Rifle_n`, `AR_shotgun_n` |
| SPECULAR | 11 | `inview_glove_s`, `TMD_new2_s`, `laser_s`, `ar_sniper_rifle_s` |
| EMISSIVE | 4 | `AR_E99Rifle_alt_e`, `TMD_new_s`, `handgun_g`, `AR_shotgun_e` |
| MASK | 4 | `AR_shotgun_rim`, `handgun_s` (spec+env mask), `AR_E99RifleLens_m`, `scope_glass_o` |
| DETAIL | 1 | `DETAILfinepitted02a_n` (a **normal** detail map) |
| SPHERICAL | 1 | `matballInview` |
| UNKNOWN | 1 | `reddot` (special reticle sample, param `reddot`) |

`UNKNOWN` is deliberate: `reddot` has no generic slot. `grey` is a 32×32 flat
utility texture (diffuse + specular on the sniper lens) and is not worth
upgrading. `handgun_s` is dual-purpose (`specularmap` + `envmask`).

## TEXTURE FORMATS

All first-person textures are **BC1 (PF_DXT1)** except three diffuse maps which
are **BC3 (PF_DXT5)**: `AR_E99Rifle_alt_d`, `AR_shotgun_d`, `TMD_new2_d`
(plus the gatling/railgun/sniper/scope diffs). No BC5 texture was found in this
set. Mip counts are consistent with `MipTailBaseIdx` 9–11 (full chains down to
the UE3 tail). `CompressionSettings` is `TC_Default` for colour/spec/mask and
`TC_Normalmap` for normals. Address mode is `TA_Wrap` everywhere.

No explicit `SRGB` flag is serialised in the cooked object; by UE3 convention
ALBEDO/EMISSIVE are treated as sRGB and NORMAL/SPECULAR/MASK as linear.

### Extraction caveat (important)

Singularity ships a single `RvGame\CookedPC\coalesced.tfc`; the packages request
caches named `Textures_P0..P3`. The pipeline hard-links `coalesced.tfc` to those
names in a private scratch dir (the community UModel workaround). **The coalesced
cache does not contain every mip of every texture.** UModel exports the
*highest-resolution mip actually present*, which is often below mip 0; some
textures abort when it tries to read a missing lower mip.

For the 55 referenced textures: **18 extract at full mip 0**, **10 only at a
lower mip**, **15 fail to export** (UModel aborts at mip > 0 — including the
pistol `handgun_*` set and `TMD_new2_d`). Manifests record `exported_dims` /
`exported_mip_level`; the validator warns when a candidate was built from mip > 0.
The three first-experiment textures are all full **mip 0**.

## NORMAL MAP ENCODING

Confirmed by direct pixel analysis of the extracted DXT data and by the material
`TC_Normalmap` setting:

* Encoding: **standard RGB tangent-space normal**, stored in **BC1/DXT1**,
  blue channel carries **Z**. `B` correlates with `sqrt(1−X²−Y²)` at r = 0.84
  (`inview_glove_n`), 0.97 (`AR_E99Rifle_n`), 0.97 (`TMD_new_n`).
* Alpha is **constant 255** → this is **not** the UE3 "X-in-alpha" packing.
* No BC5 and no DXT5 channel packing is used in this texture set.

Consequence: normals must **not** be run through a photographic SR model. The
pipeline interpolates the channels, renormalises the vectors per mip level and
re-encodes as RGB. (Green-channel orientation is left untouched; flipping it
would break lighting.)

## UPSCALE METHODS TESTED

Two conservative, **non-generative** methods were run on every first-experiment
texture, plus a technically-correct path for normals:

* **A — Lanczos-4 resample.** Windowed-sinc interpolation. Never invents content;
  the safest possible 2× for text/stencils/markings.
* **B — Iterative Back-Projection (IBP).** Starts from A, then iterates
  (8×) `HR ← HR + ↑(LR − ↓HR)` so the down-sampled high-res estimate matches the
  source. Recovers edge detail *consistent with the original*; no generative
  model, no hallucinated symbols.
* **Normals**: interpolate channels → renormalise → re-encode; A and B both go
  through renormalisation. Never a photographic upscaler.
* **Specular / masks / emissive masks**: interpolation only (Lanczos/bicubic).
  A restrained, threshold-limited unsharp helper exists for optional cleanup.

Generative SR (Real-ESRGAN / waifu2x) is deliberately deferred because of the
hallucinated-text risk on weapon markings; it may only be added behind a text-risk
review (see Risks).

## VISUAL COMPARISON

Local side-by-side crops (not committed):
`...-assets-local\textures\upscaled\<tex>__compare.png`
(original 1× / 2× nearest reference / A / B at matched pixel scale).

* `inview_glove_d` — A is a clean, faithful enlarge. B adds a little definition on
  the glove seam, belt weave and mesh panel without ringing or invented detail.
* `AR_E99Rifle_alt_d` — both preserve the fine mechanical detail and the small red
  stencil marks. B is marginally crisper; **no fake characters** appeared.
* `inview_glove_n` — both keep a coherent normal field; B slightly crisper. No
  channel/encoding damage after renormalisation.

Both A and B preserve the 2010 art style. B is *slightly* sharper; per the brief
the sharper output is **not** automatically preferred.

### Text / symbol risk register

* `AR_E99Rifle_alt_d` contains small stencil/ID marks → **A recommended**;
  any B output must be checked under the headset before adoption.
* `handgun_*`, `ar_gatling_ammo_chain_*`, `elite_beam_ammo_*`,
  `elite_beam_scope_*`, `TMD_new2_d`, `matballInview` are **higher-risk for text**
  and are not in the first experiment.

## VRAM COST

DXT bytes × full mip chain (top-mip × 4/3), from `manifests/vram_budget.json`.
2× width **and** 2× height ⇒ ≈4× pixel storage.

| group | original | 2× | increase |
| --- | --- | --- | --- |
| hands only (glove d/n/s + detail) | 2.04 MB | 8.17 MB | **+6.12 MB** |
| hands + TMD (adds TMD d/n/s/emissive) | 7.46 MB | 29.83 MB | **+22.38 MB** |
| hands + TMD + selected weapons | 21.13 MB | 84.50 MB | **+63.38 MB** |

Per first-experiment candidate:

| texture | original | 2× candidate | format |
| --- | --- | --- | --- |
| `inview_glove_d` | 0.67 MB | 2.67 MB | DXT1 |
| `inview_glove_n` | 0.67 MB | 2.67 MB | DXT1 |
| `AR_E99Rifle_alt_d` | 2.67 MB | 10.67 MB | DXT5 |

The hands-only upgrade is cheap (+6 MB) and is the clear first win. The E99
diffuse alone is +8 MB (2048×1024 → 4096×2048); it is the single largest item and
should be justified against the project's existing performance budget. Because
many textures only exist at lower mips, the *realistic* deltas are lower than the
table above, but so is the achieved quality.

## MIPMAP STRATEGY

Final candidates must carry a correct chain — VR makes bad chains visible as
shimmering, aliasing and texture crawl.

* Method: generate the top level, then successive levels with **Lanczos**
  (box-average is acceptable for masks); stop at **4×4**.
* **Normals**: renormalise the vector field at **every** level after downsampling.
* **Alpha**: distinct alpha channels are resampled with the same filter and kept.
* **Format**: keep the source container (BC1 for DXT1, BC3 for DXT5); never
  change compression on a replacement without a review.
* Chain length must match the source's (the validator checks ≈ log2(maxdim)−1).
* Production encoding should switch from the Pillow encoder to **Microsoft
  `texconv`** (`-m 0 -f BC1_UNORM / BC3_UNORM -srgbi/-srgb`) for higher-quality
  DXT and exact sRGB control; the pipeline isolates encoding in one function.

## RECOMMENDED FIRST TEXTURES

Ranked by (visibility in VR) ÷ (VRAM cost) ÷ (risk):

1. **`inview_glove_d`** — first-person glove diffuse, 1024², DXT1, full mip 0,
   ALBEDO. Highest visibility, ~+2 MB.
2. **`inview_glove_n`** — glove normal, 1024², DXT1, full mip 0. Pairs with #1;
   renormalised path proven.
3. **`inview_glove_s`** — glove specular. **Only exports at mip 2 (256²)** — do
   not fake-upscale; treat as blocked until the full mip is obtained.
4. **`AR_E99Rifle_alt_d`** — E99 rifle diffuse, 2048×1024, DXT5, full mip 0.
   Most visible weapon; +8 MB, text-risk review required.
5. **`AR_E99Rifle_n`** — E99 normal, 2048×1024, DXT1, full mip 0. Pairs with #4.

If only one change is ever shipped, ship the **hands** (1 + 2): small, safe,
always on screen.

## REPACK / INJECTION OPTIONS

Researched only — **none implemented**.

| # | option | saves | exe/pkg integrity | disable | risk | notes |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | **D3D9 runtime texture substitution** (mod-side) | safe | packages + exe untouched | one ini flag | medium | The mod already hooks `CreateTexture` / `SetTexture` / `LockRect` and tracks textures. Needs a reliable way to identify the target texture (dimensions+format+usage, and/or a content hash of the first upload). Least invasive; fully reversible. |
| 2 | **Loose cooked-package replacement** (drop-in edited `.xxx` + `.tfc`) | safe | install modified | remove file | high | UE3 needs a loader/mutator to prefer the loose package; editing cooked packages is poorly supported. Also needs a rebuilt TFC or a new cache name. |
| 3 | **`coalesced.tfc` / package rebuild** | safe | install modified | restore file | very high | Repacking cooked UE3 packages + regenerating the shared TFC is not supported by the in-repo tools; easily corrupts the install. |
| 4 | **UE3 engine-level texture hook** (substitute at `UTexture2D` load) | safe | exe untouched | ini flag | high | Powerful but needs deep engine RE (crash surface, unknown cooked streaming path). |

**Preference: option 1.** It preserves saves, does not alter the executable or
packages, can be disabled with a single ini key, and the mod's resource-hook
infrastructure already exists. Option 2 is the fallback if package identity is
required. Options 3–4 are not recommended.

## RISKS

* **Missing mips in `coalesced.tfc`** — 15/55 textures cannot be exported at all
  and 10 only at a lower mip, so a true 2× of the original top mip is not always
  possible. Mitigation: ship only full-mip-0 textures first; document the rest as
  blocked rather than fake-upscaling them.
* **AI hallucinated text/symbols** — weapon stencils and the TMD display are the
  danger zones. Mitigation: non-generative methods for the first experiment;
  text-risk review before any generative SR.
* **VRAM / performance** — the project already has performance pressure; a blind
  pack would add tens of MB. Mitigation: budget table above; start with hands.
* **Texture identity for runtime substitution** — a wrong match could retexture
  unrelated geometry. Mitigation: fingerprint (dims+format+usage) plus first-use
  ordering checks in a test build; keep an easy kill-switch.
* **Mip/alias quality in VR** — bad chains shimmer. Mitigation: correct chain
  generation + normal renormalisation + validator gates.
* **Legal** — extracted assets are copyrighted. Mitigation: pixels stay local and
  git-ignored; only metadata/hashes/scripts are committed.

## NEXT IMPLEMENTATION STEP

1. Obtain full mip 0 for the blocked high-value textures (glove specular, TMD
   diffuse, pistol) — investigate a direct TFC mip reader or a split of
   `coalesced.tfc`; do **not** fake-upscale lower mips.
2. Swap the Pillow DXT encoder for `texconv` and re-encode the three candidates
   with explicit sRGB/linear flags; re-run `validate`.
3. Add a **headset A/B capture** pass for candidate A vs B on the hands and the
   E99 rifle (the two most visible assets) before choosing.
4. Implement **option 1** (D3D9 runtime substitution) behind a single ini flag,
   default **off**, keyed by a texture fingerprint; keep it out of `d3d9.cpp`
   until reviewed. This is a *separate* work item from the current OpenXR work.
5. Only then extend to the remaining hands+TMD set (+22 MB) and the visible
   weapons.

---

## PRODUCTION SAFETY

* Production source modified: **NO** (`SingularityVR-production` untouched,
  HEAD `71fa0361…` inspected read-only).
* Game packages modified: **NO** (`CookedPC` untouched; UModel read via scratch
  hard-links only).
* VR DLL modified: **NO** (`d3d9.cpp` / runtime untouched).
* Game deployed: **NO**.
* Game textures committed: **NO** (local-only, git-ignored).
