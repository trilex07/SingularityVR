# Stereo square artifacts — decal pass fix (2026-10-12)

**Branch:** `fix/stereo-decal-artifacts` (from `726e1a7`)  
**Research (not merged):** `research/shadow-stereo-artifact` @ `7b33231`

## Root cause (evidence-backed)

| Finding | Source |
|---|---|
| ~800–840 shadow-map draws dropped; **squares remained** | `SHADOW_SFX_HANDOFF_20261008.md`, run 40 |
| `ShadowMode=1` / `2` did not remove visible squares | Research diagnosis + HISTORY |
| **High Quality Decals OFF** removes the artifact | User observation, HISTORY run 44 |
| HQ decals = **deferred projection** using depth + screen-position UV math | UE3 + `d3d9.cpp` ApplyEyeRemap comment (run 11) |
| Scene-sized **`G16R16`** target, ~**2 draws/frame** | Render-target census (run 41) |
| Dropping G16R16 draws (**NUMPAD0**) did **not** fix | HISTORY run 45 — pass is implicated but not fixed by skip |
| `ps c1` = **ScreenPositionScaleBias** (confirmed) | Runs 41–42, scope fix run 213j |

**Conclusion:** The per-eye mismatch is **not** ordinary shadow-map generation. It matches **deferred decal / depth-reconstruction** sampling a **side-by-side depth buffer** without a per-eye **ScreenPositionScaleBias** halving (same failure mode as the scope overlay, run 213g).

`ShadowMode=3` census `mask=0x0` remains a **separate** shadow-receiver issue; it does not explain HQ-decals OFF.

## Fix (opt-in, default OFF) — v2 @ `5a799ef`

`[Render] StereoDecalPassFix=1` enables a **narrow** path only when **all** match:

- Bound RT is scene-sized **`G16R16` / `G16R16F`**
- Draw shape: configured `DecalPassPrimType` + `DecalPassPrimCount` (default **TRIANGLESTRIP / 2**), or **TRIANGLELIST / 2**, or small draw with **NDC-spanning** vertex bounds
- **Not** a shadow-layout shader

Then (if `DecalPassViewportSplit` / `DecalPassPsRemap` = 1): per-eye viewport + optional **ScreenPositionScaleBias** remap on `DecalScenePsReg` (default **c1**).

With fix on, **`DecalPassAutoSplitColour=1`** (default) forces **`SplitColourTargetsOnly=1`** so G16R16 is not treated as scene-colour stereo split.

**Counters** (1 Hz in `view_matrix.log`): `hookG16`, `stereoG16`, `classified`, `fixed`, `unchanged`.

**v1 failure:** remapping **every** G16R16 draw (~240/s) removed shadows. **v2** only touches **classified** draws.

Candidate DLL: `F8D585CD9FB2C72BA6837F4327C173D850679824F531306FFF46EA77A3AF0817` in `deployment/candidate-stereo-decal-narrow-20261012/`.

## Validation status

| Gate | Status |
|---|---|
| L1 unit test `decal_stereo_pass_test.cpp` | Run after vcvars build |
| L2 PE32 `/O2` build | `850DED7E624C7911BAEAAC6E9EF3B62E1801D7080F17FBBC4D461DBEDC3F14AD` |
| L3 Steam + OpenXR | **Pending** — need log line `decal-pass: stereo fix on G16R16` + PSVR2 |
| Production default | **OFF** — accepted DLL unchanged until verified |

## Steam A/B (one variable)

1. Baseline accepted DLL `2FE5011C…`, `StereoDecalPassFix=0`.
2. Candidate with `StereoDecalPassFix=1` only; keep `ShadowMode`, `VrShadows`, `DropExtraTarget` at production values.
3. Same room/light; compare per-eye squares with HQ decals **ON**.

**False pass:** `session=0` or desktop window without stereo — not visual evidence.

## PSVR2 (if needed)

1. Load save in a lit interior with ground decals (manhole / stains).
2. Stand so squares were visible on accepted build.
3. Deploy candidate with `StereoDecalPassFix=1` only; reload.
4. **Pass:** squares match between eyes; decals still visible; no new seam flicker.
