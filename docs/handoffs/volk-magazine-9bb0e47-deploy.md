# MAIN deploy — Volk S4 carried magazine `9bb0e47`

**Date:** 2026-10-23  
**Branch:** `fix/side-volk-magazine-af580e6`  
**Commit:** `9bb0e47c42483b6fc74d1a8581be8153e0561b71`  
**Base:** `af580e6` (TMD assembly continuity — ancestor verified)

## SHA256

| Source | DLL |
|--------|-----|
| SIDE reported | `01160D846F2A4142B900A6FFD160B1F2F9B6E677AD69294D8E88E64CA3C24F78` |
| **MAIN installed** | **`A31A5C4D7D4A1D7313EDC3863EDE2DD233AB922BF49BAC0D3695BD2685BB480C`** |

PE32 `/O2` rebuild @ `9bb0e47`; commit + ancestry verified. Hash delta vs SIDE artifact is host provenance (consistent with prior MAIN/SIDE pairs).

## Rollback

`deployment/deploy-backups/20261023-pre-volk-magazine-9bb0e47/`  
- Prior DLL `C7DBEB0D…` (`af580e6`)  
- INI `00200C05…`, SVTG `452EA0C0…`

## INI (preserved)

- `ReloadPalmGrip=1`, `ReloadNativeMagazine=1` (confirmed in active Steam INI)
- TMD / dual / localization / OLED / locker settings unchanged

## Offline tests (MAIN)

- `test-tmd-short-gauntlet.ps1` (+ sleeve palette) — PASS  
- `test-dual-weapon-tmd.ps1` — PASS  
- `test-tmd-runtime-regressions.ps1` (D3D9/Ex 7110) — PASS  
- `tools/reload/test-centurion.ps1` — stopped at `test-next-weapon-runtime` (needs `-PreparedRoot`; upstream CPU reload/carry tests before that point passed)

## Runtime evidence to capture (Volk reload gameplay)

- `next reload`, `next reload gate`, `next reload draw gate`
- `Volk carry wrist: TMD material-0 published...`
- `nativeMagazineDraws`
- `tmd-assembly bound` / `tmd-assembly source`
- SVNC / `tmd-short-gauntlet: stereo receipt`

## PSVR2 (user)

| Report | PASS / FAIL |
|--------|-----------|
| Carried magazine visible in left glove | |
| Insert + ammo + fire | |
| TMD geometry regression vs `af580e6` | |

No merge to `main` without physical acceptance.
