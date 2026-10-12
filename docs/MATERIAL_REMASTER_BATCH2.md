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
injector's `targets_generated.h` on the texture-upscale line. Substituted
outputs use exactly the filename the injector looks up (`<target>_2x.dds`,
e.g. `AR_E99Rifle_alt_d_2x.dds`); the name↔hash↔file mapping is committed as
`asset-tools/textures/target_map.json` (extracted from the v2 injector table,
80 entries) and the agreement is asserted by test, not by convention.

Every substituted map is verified true mip0 by the loader (DDS header vs object
mip0 size — hard error otherwise), and every target hash already exists in the
injector's `targets_generated.h` on the texture-upscale line.

Unlike batch 1, `door_rollingT_s` **is** true mip0 (256x256) — but it is still
review-only. Substitution policy is uniform across both batches: diffuse +
normal only. Changing specular/emissive response without runtime validation
risks exactly the black-material class of surprise, so even true-mip0 specular
maps are produced for inspection, never deployed. Each exclusion carries a
recorded `exclusion_reason` in its manifest entry.

## Method (revision 2 - visual review verdict applied)

The revision-1 approach (procedural micro-detail painted onto diffuse maps)
failed visual review: on dark textures it reads as dirt and random black
stripes, and the "remastered" textures look worse than a clean upscale. The
procedural diffuse enhancers were deleted from `remaster_batch2.py`. Current
rule:

- **Substituted diffuse = faithful Lanczos 2x of the true mip0**, full mip
  chain, channels filtered independently so packed alpha survives. Zero
  invented pixels. Fidelity is measured against the clean resample, so the
  reported numbers are DXT + packaging loss only.
- **Substituted normal = micro relief derived from the NATIVE source diffuse**
  (the only genuine detail available), presented at 2x through signed-space
  Lanczos + renormalisation. `enhance_normal` slope amplitudes:
  organic 0.85, soldier 0.60, optic 0.35 (weapon 0.65, environment 0.75).
  Batch-3 procedural-only relief is unchanged.
- Specular/emissive stay review-only by uniform policy.

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

Fidelity is measured against the same-resolution clean reference
(Lanczos-2x of the source), so the numbers below are DXT + packaging loss
only. Detail ratio for diffuse is informational (output vs clean resample;
~1.1x is DXT block noise, asserted within 0.8\u20131.3 as a smoke check).

| asset | map | res | mean abs | max | detail ratio | luminance shift |
|-------|-----|-----|----------|-----|--------------|-----------|
| AR_E99Rifle_alt | diffuse | 4096x2048 | 1.51 | 41 | 1.11x | −1.6% |
| AR_E99Rifle_alt | normal | 4096x2048 | 4.12 | 164 | — | — |
| ar_gatling_gun | diffuse | 4096x2048 | 1.26 | 30 | 1.11x | −2.8% |
| ar_gatling_gun | normal | 4096x2048 | 2.49 | 136 | — | — |
| laser | diffuse | 512x512 | 1.79 | 24 | 1.10x | −5.1% |
| laser | normal | 512x512 | 4.03 | 189 | — | — |
| revert_d2 | diffuse | 4096x4096 | 1.80 | 29 | 1.14x | −0.5% |
| revert_d2 | normal | 4096x4096 | 2.83 | 112 | — | — |
| phasetick | diffuse | 1024x1024 | 3.62 | 53 | 1.15x | −0.3% |
| phasetick | normal | 1024x1024 | 6.85 | 200 | — | — |
| Spetsnaz_MP_helmet | diffuse | 2048x2048 | 1.81 | 35 | 1.12x | −0.9% |
| Spetsnaz_MP_helmet | normal | 2048x2048 | 2.40 | 154 | — | — |
| Spetsnaz_MP_Body | diffuse | 2048x2048 | 2.03 | 39 | 1.12x | −0.7% |
| Spetsnaz_MP_Body | normal | 2048x2048 | 4.86 | 215 | — | — |
| door_rollingT | diffuse | 1024x1024 | 2.54 | 54 | 1.09x | −0.5% |
| door_rollingT | normal | 1024x1024 | 3.85 | 158 | — | — |

All maps satisfy mean abs < 16 and luminance shift < 15% (asserted by
`test_remaster_batch2.py`).

Normal-map unit length, enhanced vs shipped original (% of texels outside the
0.98–1.02 tolerance — lower is better; every enhanced map beats its original):

| normal map | original outside-tol | enhanced outside-tol |
|------------|---------------------|---------------------|
| AR_E99Rifle_n | 32.3% | 17.5% |
| ar_gatling_gun_n | 20.0% | 11.9% |
| laser_n | 29.7% | 17.0% |
| revert_n2 | 31.8% | 15.3% |
| phasetick_n | 48.7% | 30.8% |
| Spetsnaz_MP_helmet_n | 29.1% | 14.4% |
| Spetsnaz_MP_Body_n | 34.4% | 19.7% |
| door_rollingT_n | 15.8% | 9.6% |

Video memory, substituted maps only (block arithmetic over measured headers):

| asset | source mip0 | replacement 2x (all mips) |
|-------|-------------|------------------------|
| weapon_e99 | 3,145,728 B | 16,777,272 B |
| weapon_gatling | 3,145,728 B | 16,777,272 B |
| weapon_laser | 65,536 B | 349,552 B |
| enemy_revert | 4,194,304 B | 22,369,648 B |
| enemy_phasetick | 262,144 B | 1,398,128 B |
| enemy_spetsnaz_helmet | 1,048,576 B | 5,592,536 B |
| enemy_spetsnaz_body | 1,048,576 B | 5,592,536 B |
| env_rollingdoor | 262,144 B | 1,398,128 B |
| **batch-2 total** | **13,172,736 B (12.56 MB)** | **70,254,864 B (67.00 MB)** |

The replacement sets are larger because they ship complete mip chains while the
UModel exports are mip0-only. No saving is claimed.

## Review-driven fixes (batch 2 revision)

Two defects found by visual review after the first batch-2 run, both fixed and
regenerated, all tests re-passing:

1. **Alpha-coupled resampling destroyed bright transparent RGB.** PIL resize
   filters (all except NEAREST) zero RGB where alpha is ~0. UE3 packs additive
   glow as bright RGB with near-zero alpha (the E99 bullet), so every mip level
   ≥ 1 and every Lanczos comparison tile erased it. Verified by pixel forensics
   (native mip0 intact: 377 gold pixels; mip1+ black). Fix: new
   `dds_io.resize_rgba_independent` (colour and alpha filtered separately),
   now used by every mip-chain, upscale and comparison-sheet path. Bullet
   survival re-verified per mip level (mip0 397/748 bright px → mip4 2/3,
   graceful decay).
2. **Directional scratches read as black bars.** Scratch strokes cut to ~1/4
   count and ~1/3 strength on weapons (349→87 strokes, 30%→10%), env score
   lines halved (28%→12%), organic cracks halved (16%→8%). Detail-energy ratios
   stay above the 1.02 gate (1.05–1.40x).

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
