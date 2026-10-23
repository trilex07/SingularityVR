# MAIN — diagnostic deploy `067e1b1` (disappearing weapon)

**Date:** 2026-10-23  
**Branch:** `fix/sol-dual-tmd-runtime-alignment`  
**Commit:** `067e1b1b7301987501354e9dab63030e26eb3d6a`  
**Base:** `cac4ce3` (ancestor verified)

## Scope (review only)

+76 lines vs `cac4ce3`: bounded **`dual-gun transition` / `matrix` / `row` / `submitted`** probes on foreground gun draws when `DualWeaponTmdProbe=1`. No geometry/input fixes.

## SHA256

| Source | DLL |
|--------|-----|
| SOL reported | `A93D60D00D5317AA5DE58D4F2E50B861472A4E2B0E4BBE55CC3AC54A84755628` |
| **MAIN installed** | **`A0A3365A34C0F957263B286CC982A3E5422AA370995183CC0F421252A0763589`** |

PE32 `/O2` @ `067e1b1`; short-gauntlet + dual-TMD + D3D9/Ex 7110 + OLED regressions **PASS**.

## Rollback after session

`deployment/deploy-backups/20261023-pre-runtime-alignment-067e1b1/` → **`BEA5A562…`** (`cac4ce3`) unless user keeps diagnostic build.

## INI

- `[Diagnostic] DualWeaponTmdProbe=1` (confirmed)
- `[Input] DualWeaponTmdReady=1`, `TmdShortGauntlet=1` unchanged
- **No** `9bb0e47` integration

## PSVR2 protocol (one session, Volk/shotgun)

1. Weapon right, TMD idle  
2. Raise left TMD (do not touch weapon)  
3. L2 Impulse  
4. R2 fire while TMD active  
5. Release TMD activation  
6. Left Grip → release  
7. Idle  

User notes **which step** weapon vanishes / returns.

## Post-session evidence (MAIN)

Archive fresh `view_matrix.log`; extract dual-gun + census + grip + handsStatus; classify A/B/C/D per handoff. Push `handoff/runtime-alignment-067e1b1-diagnostic-evidence`. Restore `cac4ce3` by default.

**No merge to `main`.**
