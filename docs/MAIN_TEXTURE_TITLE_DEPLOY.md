# MAIN runbook: texture + title deployment (SIDE-prepared, MAIN-executed)

**Status of this document:** preparation only. SIDE performed no Steam deploy,
no game launch, no injector enable, no renderer change. Every step below is
MAIN-owned and MAIN-executed.

**Blocking preconditions (from `docs/MATERIAL_REMASTER_INTEGRATION_READINESS.md`
on `feature/material-remaster-v2`, verdict CONDITIONAL NO-GO). Do not start
below until all three clear:**

1. Slot-65 double-hook ownership decided (injector `vt[65]` vs production
   `PatchVTable`), with an explicit forward chain.
2. Ex-device path resolved or test explicitly scoped to `D3D9ExMode=0`
   (injector hooks plain `CreateDevice` only; `MANAGED` replacements fail on
   Ex; `LockRect` hashing misses translated textures).
3. Black-materials hypothesis closed by the rename-off A/B on the frozen hands
   DLL (`dinput8.dll` → `dinput8.dll.disabled-by-vr`, same `d3d9.dll`).

Additional rule: finish or park the stereo shadow/decal work first. Do not
stack two rendering variables in one session.

---

## Part A — textures (18 files, 2x tier, OFFLINE-verified only)

Source of truth for hashes: per-material manifests on branch
`feature/material-remaster-batch2` (`material-remaster-batch2/manifests/*.json`,
`material-remaster-batch3/manifests/*.json`). The table below pins names,
resolutions and short SHAs; re-verify full SHA-256 against the manifests at
deploy time.

| file (into `Binaries\texture-override\`) | res | fmt | sha12 | bytes |
|---|---|---|---|---|
| `AR_E99Rifle_alt_d_2x.dds` | 4096x2048 | DXT5 | a0cdc3dd8528 | 11184848 |
| `AR_E99Rifle_n_2x.dds` | 4096x2048 | DXT1 | 54d47f3b7f15 | 5592424 |
| `ar_gatling_gun_d_2x.dds` | 4096x2048 | DXT5 | 6cc98f65b188 | 11184848 |
| `ar_gatling_gun_n_2x.dds` | 4096x2048 | DXT1 | c85bd288320e | 5592424 |
| `laser_d_2x.dds` | 512x512 | DXT1 | 3157f9bcca1f | 174776 |
| `laser_n_2x.dds` | 512x512 | DXT1 | 9f006af38243 | 174776 |
| `revert_d2_2x.dds` | 4096x4096 | DXT1 | f0f2a12427a6 | 11184824 |
| `revert_n2_2x.dds` | 4096x4096 | DXT1 | 505e0040ce7b | 11184824 |
| `phasetick_d_2x.dds` | 1024x1024 | DXT1 | 705843a836fd | 699064 |
| `phasetick_n_2x.dds` | 1024x1024 | DXT1 | 29f4d27b9caa | 699064 |
| `Spetsnaz_MP_helmet_d_2x.dds` | 2048x2048 | DXT1 | 1967f1328209 | 2796216 |
| `Spetsnaz_MP_helmet_n_2x.dds` | 2048x2048 | DXT1 | b8694c82cc7f | 2796216 |
| `Spetsnaz_MP_Body_d_2x.dds` | 2048x2048 | DXT1 | b83e6c279da1 | 2796216 |
| `Spetsnaz_MP_Body_n_2x.dds` | 2048x2048 | DXT1 | 74812b145529 | 2796216 |
| `door_rollingT_d_2x.dds` | 1024x1024 | DXT1 | 7260e2b00e09 | 699064 |
| `door_rollingT_n_2x.dds` | 1024x1024 | DXT1 | 554c50c7b15e | 699064 |
| `elite_beam_n_2x.dds` | 4096x4096 | DXT1 | a00fc23f679b | 11184824 |
| `ar_sniper_rifle_n_2x.dds` | 4096x2048 | DXT1 | 37ea2c1a3c36 | 5592424 |

Total: **87,032,112 B (~83 MB)**. Explicitly NOT deployed:
`TMD_new_n_normal_2x.nodesc.dds` (no injector target-table entry),
all `*_review.dds`, all `*_LANCZOS.dds`, all `comparison/*.png`.

Method behind these files (so MAIN knows what is being shown): substituted
diffuse = gentle unsharp (r2/20%/thr5, RGB only, alpha byte-exact) + Lanczos
2x of the true mip0, full mip chains, channels filtered independently;
normals carry micro relief + signed 2x. Fidelity vs clean resample 1.3–8.8
(mean abs), luminance shift < 6%, palettes preserved. Full metrics in
`docs/MATERIAL_REMASTER_BATCH2.md` / `MATERIAL_REMASTER_BATCH3.md`.

### A1. Preflight (MAIN)

1. `d3d9.dll` SHA-256 equals the accepted build; record it.
2. Deployment preflight creates the timestamped backup (never hand-copy).
3. Confirm the three §0 preconditions are closed and recorded.

### A2. Staged deploy

1. Deploy `dinput8.dll` + `dinput8_orig.dll` + `texture_override.ini` with
   `quality_tier=2x`, `mode_apply=0` (LOG-ONLY), and the 18 files above.
2. Launch, load a shotgun/E99 view: `texture_override.log` must show the six
   named candidate lines per material and **zero** `SUBSTITUTE` lines
   (`SUBSTITUTE` appears only with `mode_apply=1` — see the readiness doc §6).
   Any `not lockable` on a target, any unexpected match, any
   `D3DERR_INVALIDCALL` → stop, roll back, report.
3. Frame-time check vs renamed-off baseline: p95 delta ≤ 0.5 ms.
4. Only then flip to `mode_apply=1`. Expect exactly 18 `SUBSTITUTE` lines
   (one per file above), no others.

### A3. PSVR2 acceptance (perceptual, after autonomous gates pass)

- Shotgun/E99/gatling at inspection distance: crisper edges, same colours,
  no murk, no black streaks, bullet/decals intact.
- Laser sight, glove area (unchanged files — confirm no regression).
- Revert/PhaseTick/Spetsnaz at mid distance: relief reads in lighting, no
  silhouette or palette change.
- Rolling door: panels, star, stencil text unchanged; wear reads as surface.
- Global: no black materials anywhere, both eyes identical, menus unaffected.

### A4. Rollback

Rename `dinput8.dll` → `dinput8.dll.disabled-by-vr` (or `restore.ps1`, which
deletes exactly the four installed items + `texture-override\`), restore the
timestamped backup, verify original SHA-256. No package, save or registry
change is involved at any step.

---

## Part B — title text ("Singularity" → "Singularity VR")

### B0. What was proven offline (SIDE)

- The menu title is **not a texture**: no load/logo/title texture exists in
  the 114-texture extracted inventory.
- The strings exist as **GFx-fed text**: `Coalesced_INT.bin` holds standalone
  `'SINGULARITY'` and `'Singularity'` entries next to the `RUIDataStore`
  string aliases that feed the menu movies (`SPMain` et al in
  `UI_SinglePlayer_SF.xxx`, confirmed present via UModel listing).
- UModel cannot introspect Raven's `RvUIGFxMovie_*` classes and cannot export
  SWF, so **which movie/field renders the title is NOT proven offline**.
- Prepared on this branch: `spikes/view_matrix/title_replace.h`
  (exact-match allowlist, tested) + `tools/reference-tests/ui/title_replace_test.cpp`
  (13 checks incl. 11 never-touch guards). The runtime seam is NOT wired —
  that is MAIN work below.

### B1. Allowlist (exact whole-field match only)

| source field text | shown instead |
|---|---|
| `SINGULARITY` | `SINGULARITY VR` |
| `Singularity` | `Singularity VR` |

Deliberately NEVER rewritten (enforced by exact-match semantics + tests):
objectives (`Kill Barisov or use the TMD on the Singularity`), locations
(`Singularity Labs, Katorga-12`), achievements/trophies, credits, server
hostnames (`singularity-pc-live.*.demonware.net`), registry path
(`SOFTWARE\Activision\Singularity`), provider class names, process/window
titles (`Singularity (Running)` — Steam-integration risk, separate decision).

### B2. Required runtime evidence FIRST (MAIN)

1. Wire the GFx text-set seam in **log-only** mode: log every text field whose
   value exactly equals an allowlist entry, with movie + field identity.
2. Confirm the main-menu title field is among the matches (expected: SPMain).
   If the title arrives baked inside SWF (not via set-text), STOP — the hook
   approach is disproven for the title; report back instead of patching
   further (per the evidence-driven policy: two failed approaches → escalate,
   don't speculate).
3. Only then enable replacement for the evidenced field(s), INI-gated,
   default OFF.

### B3. Acceptance + rollback

- Menu screenshot before/after: title reads `SINGULARITY VR`, same typeface
  size/position/colour; nothing else on screen changed (spot-check objectives,
  location line, credits, MP server browser).
- Rollback: INI gate off (or unhook) → original strings return; no file change
  involved (hook-only path touches no game data).

---

## Part C — order and combined rollback

1. Preconditions (§0) → 2. black-materials A/B → 3. textures A1→A3 →
   4. title B2→B3. One variable at a time; each step has its own rollback
   (A4 / B3). If any step fails its gates, stop and report — do not proceed
   to the next variable.
