# Material remaster — integration readiness (SIDE findings)

**Branch:** `feature/material-remaster-v2` (this document only; no code changed)
**Date:** 2026-10-12
**Scope:** research audit only. No enable, no deploy, no Steam contact, no
`d3d9.cpp` / hands / shadow changes.

The offline prototype (`docs/MATERIAL_REMASTER.md`) is accepted as research,
NOT as an in-game visual PASS. This document answers whether MAIN should run an
isolated injector test, and under what conditions.

---

## 1. Why the injector is off in the accepted Steam build

Three independent mechanisms keep it out, all verified on this branch:

1. **Black-materials suspect (variant B).** On `feature/regression-recovery`
   (commits `91bc248`/`300a956`, not in this worktree) MAIN recorded black
   surfaces as suspecting the injector's variant-B set and bisected by
   renaming `dinput8.dll` → `dinput8.dll.disabled-by-vr` with the same
   `d3d9.dll` (`-TextureInjector Disable/Enable`). The accepted hands DLL
   (`2FE5…`, `726e1a7`) is frozen; the injector hypothesis is still open.
2. **Startup-crash pin in the deploy script.** `tools/deploy-singularityvr.ps1:198`
   states verbatim: *"37376-byte asset-tools build (B810…) has crashed at
   startup with current d3d9; keep 54784 (939E…)."* A full (non-`-DllOnly`)
   deploy therefore substitutes the stable 54784-byte backup for this branch's
   37376-byte research build. Hands deploys additionally used `-DllOnly`, which
   skips the injector files entirely (`deploy-singularityvr.ps1:288-307`).
3. **INI idle switch.** Even if the DLL loads,
   `texture_injector.cpp:538` logs `"disabled by ini - injector idle"` and
   `HookSetTexture` (`:441`) passes everything through when `enabled=0`.

Net: the accepted installation runs without injector substitution. That is a
deliberate MAIN decision, not an accident.

## 2. Conflict audit: injector vs production D3D9 VR hooks

All references are `spikes/view_matrix/d3d9.cpp` (production) vs
`asset-tools/texture-injector/texture_injector.cpp` (research) on this branch.

| # | overlap | severity | detail |
|---|---------|----------|--------|
| 1 | Double hook, vtable slot 65 | **HIGH** | Injector MinHook-hooks `vt[65]` (`texture_injector.cpp:382`); production overwrites `vt[65]` by raw patch (`d3d9.cpp:27389`). Last writer wins; the loser's trampoline points at the pre-patch target, so one hook silently stops seeing traffic or sees it out of order. No shared chain protocol exists. |
| 2 | Injector misses the Ex device | **HIGH** | Injector hooks only `IDirect3D9::CreateDevice` (vtable 16); production creates via `CreateDeviceEx` when `D3D9ExMode=1` (the default, `d3d9.cpp:27301`). Expected result: injector silently idle in the default config. |
| 3 | `CreateReplacement` uses `D3DPOOL_MANAGED` | **HIGH** | Production documents that the Ex device rejects `MANAGED` with `D3DERR_INVALIDCALL` (`d3d9.cpp:18923-18926`). Every substitution fails on the Ex path and silently falls back to the original; behaviour differs by INI flag. |
| 4 | `HashLevel0`/`MaybeCapture` `LockRect` fails on translated textures | **HIGH** | All textures are `MANAGED→DEFAULT`-translated under Ex (`d3d9.cpp:19153-19187`); non-`DYNAMIC` `DEFAULT` textures fail `LockRect(0, READONLY)`. The injector logs "not lockable" and never resolves exactly the streamed first-person targets it exists for. |
| 5 | Pointer-identity classification | **MEDIUM** | Production's menu-source check (`d3d9.cpp:19098-19101`) and `ProjectedShadowTexture` cache (`:19070`) key on the bound pointer. A substituted pointer can break menu detection or perturb the shadow census (extra `GetLevelDesc` probes, transient misclassification). Direct shadow-map substitution is unlikely (injector only handles DXT1/DXT5; shadow maps are R32F) — the risk is ordering/census noise. |
| 6 | Size/mip-chain assumptions | **MEDIUM** | The injector gates on original w/h/fmt but binds a different texture. Consumers reading back `GetLevelDesc`/`GetTexture` (menu scoring, shadow probe, atlas build) see replacement dims. Substitution is per-bind so both eyes stay consistent — size, not stereo split, is the risk. |
| 7 | Lifetime / Reset asymmetry | **MEDIUM** | Injector entries live for the process; no `Release` hook, no Reset invalidation, eviction drops the entry without `Release`. Stale-pointer reads and leaked replacements across Reset are possible. |
| 8 | Per-bind cost doubling | **LOW-MEDIUM** | Each `SetTexture` would pay injector `GetLevelDesc` + `LockRect` attempt + SRW locks on top of production's existing probes and interlocked ops. |
| 9 | Load-order fragility | **LOW** | Injector (`dinput8.dll` proxy) waits on `d3d9.dll`; production *is* `d3d9.dll`. Hook order is load-timing dependent. A second same-name `Hook_SetTexture` also confuses log/stack triage. |

Deconfliction prerequisites before any test (MAIN-owned): single owner for slot
65 with an explicit forward chain; replacement creation through the hooked
`CreateTexture` (pool translation + staging, as `VfxHostInitDevice` does);
`Release`/Reset invalidation mirroring `StageRemove`; exemption of
menu/shadow-class textures.

## 3. No enable / no deploy (statement of compliance)

- This branch ran: `verify_true_mip0.py`, `material_remaster.py`, the four test
  files, `build.ps1` (DLL compile only), and a `-NoBuild` deploy into a
  `%TEMP%` scratch directory. No `-Install`, no Steam path, no game launch.
- `tools/deploy-singularityvr.ps1` and any Steam runner were not executed.
- Production renderer, hands, shadow work and the Steam installation are
  untouched (working tree shows only this document as new; verified via
  `git status`).

## 4. Opt-in verification: accepted installation unaffected

- Remaster files live only in `material-remaster/` in this branch. The
  37376-byte `asset-tools/texture-injector/dinput8.dll` in this worktree is a
  local research build covered by the repo's `*.dll` gitignore rule — untracked,
  not committed.
- Deploying anything requires explicit MAIN-owned steps (`build.ps1 -Install`
  with a game `Binaries` path, or full `deploy-singularityvr.ps1` without
  `-DllOnly`/`-SkipTextures`). None were taken.
- The repo-default `texture_override.ini` ships `enabled=1`, but that file only
  takes effect if MAIN copies it next to a deployed `dinput8.dll`. Steam-off is
  enforced at deploy time plus the crash-guard pin (§1).

## 5. Redistribution risk review (game-derived assets in Git)

- `material-remaster/` commits 18 derived files, ~10.06 MiB (9 DDS ~4.23 MB +
  9 comparison PNG ~5.83 MB), plus 4 manifests and READMEs (23 files total).
- No original game bytes are committed: `git ls-files "*.dds"` returns only the
  9 derived files; no `.tfc`/`.upk`/`.xxx`; manifest `source_file` paths point
  outside the repo (only SHA-256/FNV hashes of originals are stored).
- Licence notes exist in three places (`material-remaster/README.md`,
  `asset-tools/texture-injector/README.md`, `docs/MATERIAL_REMASTER.md`):
  "derived from game textures owned by their publisher … review of this
  prototype only … not licensed for redistribution."
- Gaps: `.gitignore` has no `*.dds`/`*.png` rule (derived files are committable
  by default); manifests carry no licence/redistribution field.
- Recommendations before any merge toward main: add a manifest-level
  `redistribution: "review-only, not licensed"` field; consider Git LFS or
  removing the binary review artifacts from the merged tree (keep scripts +
  docs + hashes); do not expand beyond the three materials.

## 6. Minimal reversible integration proposal (for MAIN, not executed)

Test set (6 files, 2x tier only — all hashes already present in
`targets_generated.h` on the texture-upscale line):

- `AR_shotgun_diffuse_2x.dds`, `AR_shotgun_normal_2x.dds`
- `inview_glove_diffuse_2x.dds`, `inview_glove_normal_2x.dds`
- `door_blastdoorN_diffuse_2x.dds`, `door_blastdoorN_normal_2x.dds`

Preconditions (all MAIN-owned):

1. Resolve findings 2.1–2.4 (slot-65 ownership, Ex-device path, MANAGED pool,
   LockRect miss) or scope the test to `D3D9ExMode=0` with that limitation
   recorded.
2. Re-run the black-materials A/B first: `-TextureInjector Disable` vs Enable
   on the frozen hands DLL, so the open hypothesis is closed before new
   textures are introduced.
3. Deploy with `mode_apply=0` (log-only) first; inspect `texture_override.log`
   for `SUBSTITUTE` lines on the six targets and zero unexpected matches.

Test steps:

1. Backup via `deploy-singularityvr.ps1` preflight (automatic timestamped
   backup); record `d3d9.dll` SHA-256 equality before launch.
2. Deploy the six files + `texture_override.ini` (`quality_tier=2x`,
   `mode_apply=0`), launch, confirm log-only classification, then flip to
   `mode_apply=1`.
3. Fixed script: shotgun view → glove close-up → blast door; same save, same
   DLL, injector on vs renamed-off.

Rollback (fully reversible): rename `dinput8.dll` →
`dinput8.dll.disabled-by-vr` (or run `restore.ps1`, which deletes exactly the
four installed items + `texture-override\`); restore the timestamped backup;
verify original SHA-256. No package, save or registry change is involved.

## 7. Measurable gates

CPU/GPU overhead (MAIN measures; SIDE proposes thresholds):

- Per-`SetTexture` added cost: injector `GetLevelDesc` + hash + cache lookup.
  Budget: p95 frame-time delta vs renamed-off baseline ≤ **0.5 ms** at
  PSVR2-equivalent resolution; zero `LockRect` failures on the six targets in
  log-only mode (any "not lockable" on a target = stop).
- VRAM: replacement set measured at 4.23 MB total (headers + all mips);
  confirm via log `SUBSTITUTE` lines that only the six targets load.
- No new `D3DERR_INVALIDCALL` in logs; no leaked-replacement growth across a
  device reset (log `CreateTexture` vs `Release` counts if instrumented).

PSVR2 visual acceptance (perceptual, after autonomous gates pass):

- Shotgun: scratches/wear visible at inspection distance, no palette shift,
  no alpha-edge artefacts at ejection port/sights.
- Glove: weave/stitching visible on close look, no moiré at arm's length,
  no darkening of the base colour beyond the measured −4.0% mean.
- Door: panel layout and markings unchanged; rust/grime reads as surface
  variation, not geometry; no seam discontinuity at panel borders.
- Global: no black materials anywhere, no menu misdetection, both eyes
  identical (substitution is per-bind, verified by construction; confirm by
  observation).

## 8. Verdict: CONDITIONAL NO-GO

**NO-GO for a future isolated injector test until:**

1. Slot-65 double-hook ownership is decided (finding 2.1), and
2. The Ex-device path is resolved or explicitly scoped out (findings 2.2–2.4),
   and
3. The open black-materials hypothesis is closed by the rename-off A/B (§1).

If all three are satisfied, the six-file, log-first, rename-rollback test in §6
is small, reversible and measurable — **GO** under exactly those conditions.
The offline prototype itself remains accepted as research regardless.
