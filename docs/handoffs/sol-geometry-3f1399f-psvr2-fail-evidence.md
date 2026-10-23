# PSVR2 FAIL — SOL geometry candidate `3f1399f`

**Date:** 2026-10-23  
**Reporter:** MAIN (integration only; no rendering changes)  
**User verdict:** Physical PSVR2 **FAIL** (scenarios A raised-TMD and B two-hand support not accepted).

## Failed deployment

| Field | Value |
|--------|--------|
| Branch | `fix/sol-dual-tmd-geometry-rootcause` |
| Commit | `3f1399ff97e641d33c8391833a11b1a88c5c0b5d` |
| DLL SHA256 | `96375E307A64368A2E8CE765CC6DD16A38BE8C54E800B755DE777F7CA3192C43` |
| INI SHA256 | `4FC5A130DCAFA8D9DA71D48514122C9E305C95025BB1EF3D16A8461040160408` |
| Session flags | `[Input] DualWeaponTmdReady=1`, `[Diagnostic] DualWeaponTmdProbe=1`, `[Render] TmdShortGauntlet=1` |
| Prior handoff | `docs/handoffs/sol-dual-tmd-geometry-rootcause.md` on SIDE branch (geometry rootcause design) |

## Restored Steam checkpoint

| Artifact | SHA256 |
|----------|--------|
| DLL (`7e7d8208`) | `C075677AF7569BE121AB554953EB803CB822A0434A3EC7070EED882E89033BC8` |
| INI | `E89E159279F759ACC15DFF3BD8607F835FD055A8A931BF04B5E033A0B43A775F` |

Rollback source: `deployment/deploy-backups/20261023-pre-geometry-rootcause/` (verified on disk after rollback).

## Evidence bundle (local)

`deployment/crash-evidence/20261023-geometry-3f1399f-psvr2-fail/`

| File | Purpose |
|------|---------|
| `view_matrix_full.log` | Complete failed gameplay session (~2.3 MB) |
| `extract-chronological.txt` | Line-numbered extract: census, routes, grip, gauntlet, handsStatus |
| `route-grouped-material1.txt` | Grouped `dual-tmd route` for **part=1** (7704-tri identity) |
| `failed-deploy-d3d9.dll` | Failed DLL copy (gitignored; hash above) |
| `failed-deploy-SingularityVR.ini` | Failed INI copy |
| `manifest.json` | Hashes and artifact index |

**Screenshots:** No PSVR2 capture files were found on the Windows host at archive time (Pictures/Screenshots, Downloads). If the user has headset or chat attachments, add `psvr2-fail-1.png` / `psvr2-fail-2.png` to the evidence folder.

---

## Chronological timeline (log line numbers)

Session attach: `=== view_matrix attached 2026-10-23 11:37:26` (first segment; continued through gameplay).

| Order | Line | Event |
|-------|------|--------|
| 1 | 649 | `hands: mPlayerHandsStatus = 2` (weapon in hands) |
| 2 | 6090 | `hands: mPlayerHandsStatus = 0` (neither) |
| 3 | 11179 | `hands: mPlayerHandsStatus = 1` (**TMD RAISED**) |
| 4 | 11223–11260 | Gauntlet admitted to independent palette; **`stereo receipt eyes=2 tris=7110`** |
| 5 | 18851 | **`two-hand grip: ACQUIRED`** (first support session) |
| 6 | 18855 | Route probe: **part=1 `route=zero`** `segments=0` with `support=1` `hands=1` |
| 7 | 18931 | **`two-hand grip: RELEASED`** |
| 8 | 19019 | **`two-hand grip: ACQUIRED`** (second) |
| 9 | 19023 | `hands: mPlayerHandsStatus = 2` (weapon in hands during support) |
| 10 | 19025 | `support grip pose: authored-pair gun=8297 leftDelta=primary` |
| 11 | 19027 | **part=1 `route=zero`** `hands=2` `support=1` |
| 12 | 19128 / 19140 / 19211 | Grip **RELEASED** → **ACQUIRED** → **RELEASED** |
| 13 | 19224 | `tmd-short-gauntlet: first failed gate=focus` (7704 draw identity) |

**Weapon presentation (census, not pixels):** With `hands=2` and weapon equipped, **`part=gun`** consistently shows `gunRide≈1350` and stereo counts on gun section; **`part=hardware`** stays `gunRide=0` but often `raw=0` until raised-TMD gameplay (e.g. frame 4500: `hands=1` hardware `raw=616 reachedStereo=616`).

During **support=1** at frame 5400: `hands=1` hardware `raw=1069 reachedStereo=1069`; `hands=2` hardware `raw=192 reachedStereo=192` — hardware section draws are counted in census while route probe reports **zero** segments for part=1 at support transitions.

---

## `dual-tmd route` — material 1 outcomes (grouped)

Draw identity: `min=2626 verts=4818 first=13206 tris=7704` (mixed hardware + forearm section).

| hands | support | route | palette | segments | stereoEyes | frame (one-shot) |
|-------|---------|-------|---------|----------|------------|------------------|
| 0 | 0 | native-full | 0 | 1 | 0 | 9521 |
| 0 | 0 | hidden | -1 | -1 | 0 | 9524 |
| 0 | 0 | hardware | 45 | 1 | 0 | 9646 |
| 1 | 0 | native-full | 0 | 1 | 0 | 9652 |
| 1 | 0 | **svtg** | 45 | 1 | **1** | 9660 |
| 1 | 0 | hidden | -1 | -1 | 0 | 10037 |
| 1 | 0 | hardware | 45 | 1 | 0 | 10179 |
| 1 | 1 | **zero** | 45 | **0** | 0 | 10642 |
| 2 | 1 | **zero** | 45 | **0** | 0 | 10835 |

**Material 1 reached:** `native-full`, `hidden`, `hardware`, **`svtg`** (raised TMD, `stereoEyes=1` at frame 9660), and **`zero`** (both support captures with `segments=0`). No material-1 `dual-tmd route` row logged as `native-full` after the TMD-raised SVTG receipt window; subsequent part=1 rows are `hidden`, `hardware`, or `zero`.

Part 0 (glove material 0) at support: `route=native-full` `palette=108` (frames 10642, 10835).

---

## `tmd-short-gauntlet` gate sequence (7704 vs 7110)

| Line | Message |
|------|---------|
| 221 | CPU ready `tris=7110` |
| 9753 / 9768 | `first failed gate=animated` / `stereo` on **7704** draw (pre-raise) |
| 11260 | **`stereo receipt eyes=2 tris=7110`** (short gauntlet path succeeded once) |
| 17180 | `first failed gate=context` (7704) |
| 19224 | `first failed gate=focus` (7704) |

Interpretation for SOL: PSVR2 failure occurs in a session where **7110 SVTG stereo receipt exists**, but **7704-index identity** still appears in gauntlet gate failures and in route probe transitions; **support grip forces part=1 to `zero`** (intentional hide per geometry branch) while part=0 remains `native-full`.

---

## SOL follow-up (no MAIN geometry work)

1. Reconcile **visible defect** with route table: is the user seeing **part=0 native-full**, residual **hardware** (`palette=45`, `stereoEyes=0`), or **gunRide** on part=gun during `hands=1` raised TMD?
2. **Support grip:** `authored-pair … leftDelta=primary` still logged at line 19025 — confirm whether fore-end retarget fired for this weapon class on `3f1399f`.
3. **`route=zero` + census hardware draws** during `support=1`: expected hide vs incorrect left-hand presentation for scenario B.
4. Push a **new candidate**; MAIN stays on deployment hold until PSVR2 PASS on A and B separately.

**Do not merge to `main`.**
