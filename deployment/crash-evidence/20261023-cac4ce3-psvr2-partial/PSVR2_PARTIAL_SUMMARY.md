# PSVR2 PARTIAL — `cac4ce3` TMD component / support

**Date:** 2026-10-23  
**Reporter:** MAIN (archive only; no code changes)  
**User verdict:** **PARTIAL** — socket/assembly progress; **critical dual-weapon regression remains**

## Deployed artifacts (preserved on Steam)

| Item | Value |
|------|--------|
| Commit | `cac4ce3701c08cca28bfdcb6cbbdf189bcc5076e` |
| Branch | `fix/sol-tmd-component-support` |
| DLL SHA256 | `BEA5A5623E2995C5DEE85AC30D1736F0B3E3CEAF68AF20A84B86B7DA3E83D581` |
| INI SHA256 | `00200C0572C99D1656E294C1F312D104F27A9B4CB44BD5CD0890AEAB22387A79` |
| SVTG | `452EA0C0DDFB4721A7F9B9718629975CF021F9E079E1CD92235C076E9E376192` |
| INI flags | `DualWeaponTmdReady=1`, `DualWeaponTmdProbe=1`, `TmdShortGauntlet=1` |

**Deployment hold:** no new experimental deploy until SOL pushes a candidate.

## Preserved rollback checkpoints (unchanged)

| Label | DLL | Purpose |
|--------|-----|---------|
| `20261023-pre-component-support-cac4ce3` | `A31A5C4D…` | Volk magazine `9bb0e47` (pre-component) |
| `20261023-pre-volk-magazine-9bb0e47` | `C7DBEB0D…` | `af580e6` assembly (pre-Volk fix) |
| `20261023-pre-assembly-continuity-af580e6` | `30895A1E…` | Sleeve palette `685e42dc` build |

**MAIN will not integrate `9bb0e47` into TMD branch** — SOL owns merge after geometry acceptance.

## Screenshots

User-referenced PSVR2 screenshots were **not found** on disk at archive time (Pictures/Screenshots, Downloads, chat transcript attachments). Drop files here as `psvr2-partial-1.png`, `psvr2-partial-2.png`, … if available.

## Evidence files

| File | Description |
|------|-------------|
| `view_matrix_full.log` | Full session (~133k lines) |
| `timeline-extract.txt` | 712 line-numbered events |
| `deployed-d3d9.dll` / `deployed-SingularityVR.ini` | Failed-session copies |
| `manifest.json` | Hashes |

---

## Chronological timeline (log line numbers)

### Session setup
| Line | Event |
|------|--------|
| 1407–1408 | `hands=2` (weapon), `mTMD` acquired |
| 4898–4945 | Volk latch: `gun=8297`, `arms=3314` — healthy dual-weapon baseline |
| 5172–5177 | `dual-tmd census` `hands=2`: gun `gunRide=1251`, hardware `raw=0` |

### TMD activation + socket correction receipts
| Line | Event |
|------|--------|
| 55691 | **`hands=1` (TMD RAISED)** |
| 55814–55858 | **`tmd-attachment: cooked socket mesh CRC verified`** (729/251/207/118 face counts; shared left controller ride) |
| 55889 | **`stereo receipt eyes=2 tris=7110 sleevePalette=7`** index/palette restore OK |
| 55739 | `dual-tmd route` `hands=1` mat1 **`route=svtg`** `stereoEyes=1` |

### Dual-weapon + Grip (regression window)
| Line | Event |
|------|--------|
| 56479 | `hands=2` (weapon in hands) |
| 56498 | `hands=2` mat1 **`route=svtg`** `stereoEyes=2` |
| 56664 / 56677 | **`two-hand grip: ACQUIRED` / `RELEASED`** |
| 56675–56677 | Grip `support=1`: mat0 `native-full`, mat1 **`route=zero`** `segments=0` (intentional hide) |

### Weapon disappearance — mesh latch misclassification (critical)
| Line | Event |
|------|--------|
| 127322–127323 | Weapon swap to **`ARMachineGun`**; log notes TMD **HIDDEN rather than swapped** |
| 127324 | Swap fg census **9 entries** includes **`7444` (TMD glove), `4818`, socket `729/251/207/118`** alongside `8297` gun |
| **127341–127342** | **`mesh latch: learned that 7444 verts is the ARMS`** → **`gun=11387`, `arms=7444`** |
| 127343 | New fg baseline **10 meshes** with TMD body parts in pass |

**Hypothesis for SOL (log-supported, not pixel-proven):** With `DualWeaponTmdReady=1`, raised TMD / socket draws expand the first-person pass. On weapon swap, **Run-226 latch treats 7444-vert TMD glove as ARMS**, permanently excluding it from gun role and corrupting dual-weapon presentation (user-reported **weapon disappearance**). This is separate from socket CRC ride success at `hands=1`.

### Knife scenarios
| Lines (samples) | Event |
|-----------------|--------|
| 322–323, many | `hip knife: gate blocked` — `vis=0`, menus/cinematics/wheel often set |
| 76580+ | Occasional `vis=1` with `wheel=1` still blocked |

No successful hip-knife draw receipt in extract; gates dominate log.

---

## Runtime receipts summary

| Signal | Present in session? |
|--------|---------------------|
| Socket CRC verified | **Yes** (L55814+) |
| SVTG + `sleevePalette=7` | **Yes** (L55889) |
| `tmd-assembly bound` SVNC+SVTG same frame | **Yes** (earlier smoke; see full log) |
| `dual-tmd route` support mat1 zero | **Yes** (L56676) |
| Mesh latch 7444→ARMS on swap | **Yes** (L127341) — **dual-weapon regression** |

---

## SOL follow-up (no MAIN geometry work)

1. **Dual-weapon regression:** Prevent TMD material-0 / socket attachment meshes from entering **Run-226 arms/gun latch** when `DualWeaponTmdReady=1`, or re-latch after swap without classifying 7444 as arms.
2. Correlate user **PARTIAL** (attachments improved?) with CRC receipts vs remaining detached part in screenshots (when provided).
3. **Grip:** mat1 `zero` during support expected; wrist/glove issues remain separate.
4. **Knife:** only gate-block evidence in this log — no change in this deployment scope.
5. Push new candidate; MAIN: build → backup → deploy → PSVR2 only.

**No merge to `main`.**
