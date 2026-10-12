# Singularity VR offline material remaster — batch 3 (normal-only)

**Branch:** `feature/material-remaster-batch2` (shared with batch 2)
**Status:** OFFLINE ONLY — NOT VR TESTED. No Steam run, no deploy, no injector
enable, no headset session.
**Rule:** DDS/PNG artifacts live **outside the repository** under
`<assets-local>/material-remaster-batch3/`. Committed: tools, tests,
safe manifests (hashes + measurements, no pixels), docs.

Batch 2 exhausted the true-mip0 diffuse+normal pairs in the local inventory.
Batch 3 covers the three remaining true-mip0 maps: normals whose diffuses exist
only as lower mips.

## Batch-3 groups (3, normal-only)

| key | asset | material | source res | injector target |
|-----|-------|----------|------------|-----------------|
| `weapon_sniper_normal` | ar_sniper_rifle_n | AR_SniperRifle | 2048x1024 DXT1 | present, hash match |
| `weapon_railgun_normal` | elite_beam_n | AR_Railgun_Inview | 2048x2048 DXT1 | present, hash match |
| `device_tmd_normal` | TMD_new_n | tmd_base_optimized | 2048x1024 DXT1 | **MISSING — needs MAIN-side table update** |

Two of three are drop-in deployable under the `<target>_2x.dds` naming the
injector looks up (asserted by test). `TMD_new_n` has no entry in the injector
target table, so even a perfect replacement could never match — its file keeps
a descriptive `.nodesc.dds` name and is review-only until MAIN adds the target.
This is asserted, not assumed (`test_output_filenames_match_injector_lookup`
has an explicit TMD branch).

## Weaker provenance, labelled as such

With no true-mip0 diffuse to derive relief from, micro relief here is
**procedural-only** (fbm perturbation in signed space + renormalisation via
`enhance_normal_procedural`), not diffuse-coupled. Every manifest records this
in `newly_authored`, and the batch-3 test asserts the `procedural-only` label
is present. Diffuse maps are not processed and not substituted; each manifest's
`unprocessed` section names the lower-mip diffuse and why.

## Metrics (measured from decoded files; see manifests)

Fidelity (centre-crop mean absolute difference) and normal-map unit length
(enhanced vs shipped original, % outside 0.98–1.02 — lower is better):

| normal map | substitute size | mips | bytes | mean abs vs clean 2x | max | orig out-tol | enh out-tol |
|------------|-----------------|------|-------|----------|-----|--------------|-------------|
| ar_sniper_rifle_n | 4096x2048 DXT1 | 13 | 5,592,424 | 6.85 | 188 | 24.1% | 14.0% |
| elite_beam_n | 4096x4096 DXT1 | 13 | 11,184,824 | 7.49 | 165 | 35.3% | 17.2% |
| TMD_new_n | 4096x2048 DXT1 | 13 | 5,592,424 | 8.13 | 239 | 47.3% | 20.8% |

Revision 2 presents batch-3 normals at 2x like the rest (signed-space Lanczos
+ renorm of the procedural-relief native). Fidelity is measured against the
clean 2x resample, so the numbers are relief delta + DXT loss.

Mean abs sits higher than batch-2 normals because procedural relief
is not anchored to diffuse detail and touches more texels; it stays inside the
< 16 fidelity bound, and every map beats its shipped original on unit-length
tolerance.

Video memory, substituted maps only: source mip0 **4,194,304 B (4.00 MB)** →
replacement 2x **22,369,672 B (21.33 MB)** (5,592,424 + 11,184,824 + 5,592,424).
No saving claimed - this is the price of the 2x tier, stated up front.

Normal-map unit length is gated relatively (enhanced out-of-tolerance % must
not exceed the shipped original's), exactly as in batch 2.

## What remains after batch 3

The local true-mip0 inventory is now exhausted for substitutable colour data:

| leftover | reason it stays out |
|----------|---------------------|
| masks, opacity maps (scope_glass_o, revert_t) | semantic risk without runtime validation |
| emissive/fx maps | light-energy semantics; review-only |
| FX sprites, decals, blood | baked-lighting / energy-altering risk |
| env captures, NOLOCAL doors | no object-size provenance; mip0 unprovable |
| handgun (pistol) | captured lower mips; needs MAIN-owned runtime capture at true mip0 |

Next expansion needs either new extractions (UModel/TFC work, MAIN-owned game
area) or the pistol runtime-capture path. No further batches are planned from
the current inventory.

## Reproduce (offline)

```powershell
python asset-tools/textures/remaster_batch3.py          # writes to <assets-local>, never to the repo
python asset-tools/textures/tests/test_remaster_batch3.py
```
