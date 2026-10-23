# Deploy `685e42dc` — SVTG sleeve palette fix

**Date:** 2026-10-23  
**Branch:** `fix/sol-tmd-svtg-sleeve-palette`  
**Commit:** `685e42dc2175e2d1e652327678079f617d1c6724`

## SHA256 provenance

| Source | SHA256 |
|--------|--------|
| SIDE reported | `62F02EB20CCC07F0028375092BAA2E87AC49D0344DF2A222E7F97807578737B8` |
| **MAIN installed** (clean tree @ `685e42dc`, x86 PE32 `/O2`) | **`30895A1EA028EF7225C2924428DFCC08C89365317D9FBF9DF65829E45F10EF65`** |

Commit identity verified; offline regressions PASS on MAIN build. Hash delta is build-environment provenance (no source diff vs `685e42dc`).

## Rollback

`deployment/deploy-backups/20261023-pre-svtg-sleeve-palette/` — DLL `C075677A…`, INI `E89E1592…`

## INI (diagnostic)

- `TmdShortGauntlet=1` (`[Render]`)
- `DualWeaponTmdReady=1` (`[Input]`)
- `DualWeaponTmdProbe=1` (`[Diagnostic]`)
- Active INI SHA: `00200C0572C99D1656E294C1F312D104F27A9B4CB44BD5CD0890AEAB22387A79`

## PSVR2

Pending user Test A / Test B.
