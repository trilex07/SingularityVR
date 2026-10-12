# Singularity VR offline material remaster — batch 2

**Branch:** `feature/material-remaster-batch2`
**Status:** OFFLINE ONLY — NOT VR TESTED. No Steam run, no deploy, no injector
enable, no headset session.
**Rule:** comparison PNGs and DDS files live **outside the repository** under
`<assets-local>/material-remaster-batch2/`. This branch commits tools, tests,
safe manifests (hashes + measurements, no pixels) and docs only.

Batch 1 (shotgun, glove, blast door) is documented in `docs/MATERIAL_REMASTER.md`
on `feature/material-remaster-v2` and is not repeated here.

## Batch-2 groups (8)

| key | asset | kind | maps | source res | container |
|-----|-------|------|------|------------|-----------|
| `weapon_e99` | AR_E99Rifle_alt | weapon | diffuse + normal | 2048x1024 | DXT5 / DXT1 |
| `weapon_gatling` | ar_gatling_gun | weapon | diffuse + normal | 2048x1024 | DXT5 / DXT1 |
| `weapon_laser` | laser (sight housing) | optic | diffuse + normal | 256x256 | DXT1 |
| `enemy_revert` | revert_d2 | organic | diffuse + normal | 2048x2048 | DXT1 |
| `enemy_phasetick` | phasetick | soldier | diffuse + normal (+spec/emis review) | 512x512 | DXT1 |
| `enemy_spetsnaz_helmet` | Spetsnaz_MP_helmet | soldier | diffuse + normal | 1024x1024 | DXT1 |
| `enemy_spetsnaz_body` | Spetsnaz_MP_Body | soldier | diffuse + normal | 1024x1024 | DXT1 |
| `env_rollingdoor` | door_rollingT | environment | diffuse + normal (+specular review) | 512x512 / 256x256 | DXT1 |

Every substituted map is verified true mip0 by the loader (DDS header vs object
mip0 size — hard error otherwise), and every target hash already exists in the
injector's `targets_generated.h` on the texture-upscale line.

Unlike batch 1, `door_rollingT_s` **is** true mip0 (256x256) — but it is still
review-only. Substitution policy is uniform across both batches: diffuse +
normal only. Changing specular/emissive response without runtime validation
risks exactly the black-material class of surprise, so even true-mip0 specular
maps are produced for inspection, never deployed. Each exclusion carries a
recorded `exclusion_reason` in its manifest entry.

## What is new vs batch 1

- Three new diffuse enhancers in `asset-tools/textures/remaster_batch2.py`:
  **organic** (chitin mottling, pores, cracks, lesion tint),
  **soldier** (zero-mean uniform weave, armor scuff highlights, recess grime),
  **optic** (fine grain only — housings must stay smooth).
- `enhance_normal` slope amplitudes extended in `material_remaster.py`:
  organic 0.85, soldier 0.60, optic 0.35 (weapon 0.65, environment 0.75 unchanged).
- One genuine 2x showcase variant (`weapon_e99` only): Lanczos from the
  enhanced native replacement (signed-space 2x for the normal + renorm),
  files named `*_2X_LANCZOS.dds`, manifest-flagged `substituted: False` with an
  explicit honesty note that content above the original Nyquist frequency is
  interpolation. NOT wired into the injector.

## Deferred with reasons (not silently dropped)

| material | reason |
|----------|--------|
| handgun_d / handgun_n (pistol) | runtime captures at 1024px, object is 2048px — lower mips; needs MAIN-owned runtime capture at true mip0. Highest-priority FP weapon, explicitly deferred. |
| ar_sniper_rifle_d / elite_beam_d | diffuse exported one mip lower; normals alone deferred to a normal-only batch |
| TMD_new_n | normal is true mip0 2048x1024 but no diffuse exists at mip0; normal-only batch material |
| scope_glass | no normal map; `_o` is a red-dot opacity mask — too risky without runtime validation |
| muzzleFlashDir_01, brightFlash_01, muzzleflashCenter | additive glow sprites; micro-detail risks altering light energy; deferred pending MAIN guidance |
| env_door_a/b, env_* captures | no object-size provenance in metadata; mip0 cannot be proven — skipped, never upscaled-and-claimed |
| door_industrial / prison / locked | no local exports; extraction deferred |

## Metrics (measured from decoded files; see manifests)

Fidelity (centre-crop mean absolute difference, 0–255 levels), micro-detail
gain (high-pass energy ratio, must exceed 1.02), and palette shift:

| asset | map | mean abs | px>4% | detail ratio | luminance shift |
|-------|-----|----------|-------|--------------|-----------|
| AR_E99Rifle_alt | diffuse | 3.35 | 29.5% | 1.18x | −8.6% |
| AR_E99Rifle_alt | normal | 4.00 | 29.9% | — | — |
| ar_gatling_gun | diffuse | 4.97 | 46.8% | 1.43x | −11.7% |
| ar_gatling_gun | normal | 2.80 | 19.1% | — | — |
| laser | diffuse | 2.06 | 13.7% | 1.10x | −5.8% |
| laser | normal | 3.26 | 24.6% | — | — |
| revert_d2 | diffuse | 4.66 | 49.4% | 1.10x | −2.5% |
| revert_d2 | normal | 2.94 | 23.1% | — | — |
| phasetick | diffuse | 7.07 | 63.7% | 1.40x | −0.7% |
| phasetick | normal | 7.16 | 59.1% | — | — |
| Spetsnaz_MP_helmet | diffuse | 4.62 | 46.3% | 1.23x | −5.2% |
| Spetsnaz_MP_helmet | normal | 3.14 | 24.3% | — | — |
| Spetsnaz_MP_Body | diffuse | 4.37 | 43.9% | 1.21x | −5.1% |
| Spetsnaz_MP_Body | normal | 4.00 | 32.5% | — | — |
| door_rollingT | diffuse | 5.58 | 51.8% | 1.15x | +3.7% |
| door_rollingT | normal | 4.13 | 27.7% | — | — |

All maps satisfy mean abs < 16 and luminance shift < 15% (asserted by
`test_remaster_batch2.py`).

Normal-map unit length, enhanced vs shipped original (% of texels outside the
0.98–1.02 tolerance — lower is better; every enhanced map beats its original):

| normal map | original outside-tol | enhanced outside-tol |
|------------|---------------------|---------------------|
| AR_E99Rifle_n | 32.3% | 17.5% |
| ar_gatling_gun_n | 20.0% | 12.0% |
| laser_n | 29.7% | 17.0% |
| revert_n2 | 31.8% | 15.4% |
| phasetick_n | 48.7% | 30.8% |
| Spetsnaz_MP_helmet_n | 29.1% | 14.4% |
| Spetsnaz_MP_Body_n | 34.4% | 19.7% |
| door_rollingT_n | 15.8% | 9.6% |

Video memory, substituted maps only (block arithmetic over measured headers):

| asset | source mip0 | replacement (all mips) |
|-------|-------------|------------------------|
| weapon_e99 | 3,145,728 B | 4,194,360 B |
| weapon_gatling | 3,145,728 B | 4,194,360 B |
| weapon_laser | 65,536 B | 87,408 B |
| enemy_revert | 4,194,304 B | 5,592,432 B |
| enemy_phasetick | 262,144 B | 349,552 B |
| enemy_spetsnaz_helmet | 1,048,576 B | 1,398,128 B |
| enemy_spetsnaz_body | 1,048,576 B | 1,398,128 B |
| env_rollingdoor | 262,144 B | 349,552 B |
| **batch-2 total** | **13,172,736 B (12.56 MB)** | **17,563,920 B (16.75 MB)** |

The replacement sets are larger because they ship complete mip chains while the
UModel exports are mip0-only. No saving is claimed.

## Reproduce (offline)

```powershell
python asset-tools/textures/remaster_batch2.py          # writes to <assets-local>, never to the repo
python asset-tools/textures/tests/test_remaster_batch2.py
python -m pytest asset-tools/textures/tests/test_dds_io.py asset-tools/textures/tests/test_normal_map.py -q
```

## Tests

| test | what it proves |
|------|----------------|
| `tests/test_remaster_batch2.py` (10 checks) | safe manifests cover all 8 groups; **no DDS/PNG committed** (git-enforced); substituted maps are true mip0; outputs decode with original res + container; normal conventions; fidelity bounds + detail gain; luminance shift < 15%; showcase variants labelled non-substitute; evidence labels present |
| `tests/test_dds_io.py`, `tests/test_normal_map.py` | shared DDS/normal infrastructure (unchanged from batch 1) |
