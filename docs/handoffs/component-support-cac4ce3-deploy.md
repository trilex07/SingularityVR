# MAIN deploy — SOL TMD component / support `cac4ce3`

**Date:** 2026-10-23  
**Branch:** `fix/sol-tmd-component-support`  
**Commit:** `cac4ce3701c08cca28bfdcb6cbbdf189bcc5076e`  
**Handoff:** `docs/handoffs/sol-tmd-socket-correction.md`

## Ancestry

- `685e42dc` (sleeve palette): **yes**
- `af580e6` (glove alignment / assembly): **yes**
- **`9bb0e47` Volk magazine:** **not included** (do not cherry-pick for this test)

## SHA256

| Source | DLL |
|--------|-----|
| SOL reported | `B3998713635DDBA6B97E0B485D811ECB9B06C321279CA572F5D09FB12AA24C27` |
| **MAIN installed** | **`BEA5A5623E2995C5DEE85AC30D1736F0B3E3CEAF68AF20A84B86B7DA3E83D581`** |

PE32 `/O2` @ `cac4ce3`; regressions PASS including `test-tmd-attachment.cpp` (11 CRC identities).

## Artifacts

| Item | SHA256 |
|------|--------|
| INI (preserved) | `00200C0572C99D1656E294C1F312D104F27A9B4CB44BD5CD0890AEAB22387A79` |
| SVTG (unchanged) | `452EA0C0DDFB4721A7F9B9718629975CF021F9E079E1CD92235C076E9E376192` |

## Rollback

- Immediate prior: `deployment/deploy-backups/20261023-pre-component-support-cac4ce3/` (DLL `A31A5C4D…` = Volk `9bb0e47`)
- TMD-only baseline: `20261023-pre-volk-magazine-9bb0e47` or `20261023-pre-assembly-continuity-af580e6` (DLL `C7DBEB0D…` = `af580e6`)

## Runtime evidence (gameplay required)

- `tmd-attachment: cooked socket mesh CRC verified`
- `tmd-assembly bound` / `tmd-assembly source`
- `tmd-short-gauntlet: stereo receipt` (+ `sleevePalette=7`)
- `dual-tmd route`, `two-hand grip`

Log receipts ≠ visible fix.

## PSVR2 (user)

| Test | Result |
|------|--------|
| **A** TMD assembly (Volk + raised TMD) | PASS / PARTIAL / FAIL |
| **B** Two-hand Grip (shotgun support) | PASS / PARTIAL / FAIL |
| **C** Regressions | PASS / FAIL |

Magazine placement remains a separate track.

No merge to `main` without physical acceptance.

After geometry acceptance, SOL integrates `cac4ce3` with `9bb0e47` under SOL ownership.
