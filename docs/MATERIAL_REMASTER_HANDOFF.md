# Material remaster — handoff to MAIN

**Verdict: CONDITIONAL NO-GO** (unchanged). Prototype accepted as research only.

## To run an isolated injector test, MAIN must first close these three

1. **Slot-65 ownership** (`readiness §2.1`): injector MinHook-hooks device
   vtable slot 65; production overwrites the same slot by raw patch. Decide a
   single owner with an explicit forward chain.
2. **Ex-device path** (`readiness §2.2–2.4`): production creates the device via
   `CreateDeviceEx` by default; the injector hooks only plain `CreateDevice`,
   creates replacements as `D3DPOOL_MANAGED` (rejected on Ex), and its
   `LockRect`-based hash misses every translated texture. Fix the path or
   explicitly scope the test to `D3D9ExMode=0`.
3. **Black-materials A/B** (`readiness §1`): rename-off vs enable on the frozen
   hands DLL, before introducing new textures.

## Test package (ready, verified, not deployed)

- 6 injectable maps, 2x tier, 4,195,176 B total:
  `AR_shotgun_diffuse_2x.dds`, `AR_shotgun_normal_2x.dds`,
  `inview_glove_diffuse_2x.dds`, `inview_glove_normal_2x.dds`,
  `door_blastdoorN_diffuse_2x.dds`, `door_blastdoorN_normal_2x.dds`
  (in `material-remaster/`, with manifests + comparison sheets).
- Specular excluded (sources are lower mips); review-only files exist and are
  refused by the deploy step.
- Log-only first: with `mode_apply=0`, matches appear as **named candidate
  lines**, never `SUBSTITUTE` (code: `texture_injector.cpp:469,488,497-499`).
  `SUBSTITUTE` appears only with `mode_apply=1` after a real replacement is
  built. Rollback: rename `dinput8.dll` → `dinput8.dll.disabled-by-vr`.

## Housekeeping owned by MAIN

- **Asset removal:** `docs/MATERIAL_REMASTER_ASSET_REMOVAL_PLAN.md` — stage 1
  (`git rm` binaries) anytime; stage 2 (history purge) needs coordination.
- **Do not merge** this branch toward main with binaries in it.
- SIDE work stops here; no injector fixes will be implemented from this side
  while MAIN is on shadow/decal artifacts.

Full detail: `docs/MATERIAL_REMASTER.md` (prototype),
`docs/MATERIAL_REMASTER_INTEGRATION_READINESS.md` (audit).
