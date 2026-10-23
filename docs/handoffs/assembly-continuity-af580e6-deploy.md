# MAIN deploy — TMD assembly continuity `af580e6`

**Date:** 2026-10-23  
**Branch:** `fix/sol-tmd-assembly-continuity`  
**Commit:** `af580e654f05b85a961018b7e4263f8148507624`  
**Ancestry:** includes sleeve-palette fix `685e42dc`

## SHA256

| Source | DLL |
|--------|-----|
| SIDE reported | `4329FE7CE7F2A9A6A5801297FC5A905507F7BFE3EBD6857B2B28A21453C4BE82` |
| **MAIN installed** | **`C7DBEB0DE685CBC4AE828A222055A21448210F3ADF0B4FD63A4311D8023C88B2`** |

Clean tree @ `af580e6`, x86 PE32 `/O2` (machine `0x014C`). Commit verified; regressions PASS. Hash delta vs SIDE artifact is host rebuild provenance (same pattern as `685e42dc` deploy).

## Artifacts

| Item | SHA256 |
|------|--------|
| INI (unchanged) | `00200C0572C99D1656E294C1F312D104F27A9B4CB44BD5CD0890AEAB22387A79` |
| SVTG (unchanged) | `452EA0C0DDFB4721A7F9B9718629975CF021F9E079E1CD92235C076E9E376192` |

Rollback: `deployment/deploy-backups/20261023-pre-assembly-continuity-af580e6/` (prior DLL `30895A1E…` sleeve-palette build).

## Offline tests

- `test-tmd-short-gauntlet.ps1` (+ sleeve palette) — PASS  
- `test-dual-weapon-tmd.ps1` — PASS  
- `test-tmd-runtime-regressions.ps1` (D3D9/Ex 7110) — PASS  
- OLED suite — PASS  

## Runtime evidence (MAIN smoke — not PSVR2)

After Steam launch, grep `%LOCALAPPDATA%\SingularityVR\view_matrix.log` for:

- `tmd-assembly bound` / `tmd-assembly source`
- `tmd-short-gauntlet: stereo receipt` (+ `sleevePalette=7` when raised)
- `dual-tmd route`, `two-hand grip`

Menu/smoke alone may not emit assembly bound receipts; **raised TMD + Grip gameplay required**.

## PSVR2 gate (user)

| Test | Record |
|------|--------|
| **A** Glove + TMD continuity | PASS / PARTIAL / FAIL |
| **B** Grip wrist closure | PASS / PARTIAL / FAIL (not claimed fixed by this commit) |
| **C** Regressions | PASS / FAIL |

No merge to `main` without physical acceptance.
