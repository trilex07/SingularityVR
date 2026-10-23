# MAIN deployment — SOL P0–P3 candidate (`3929868`)

**Branch:** `fix/sol-bc3ab34-runtime-evidence`  
**HEAD:** `392986821eb544b96f17bfe2606b467d8ec24049`  
**Base:** `bc3ab34` (ancestor verified)  
**SOL reference DLL:** `9B22770F44C3931EAD88332D61B18843128972110B7AA2317E385FB5619D6D69`  
**MAIN rebuilt DLL (Steam installed):** `56BBC470E1ADBB128AB409EEAE5242B59772F8E28ED9A697269DB0CA89816B61`  
**Build:** x86 PE32 `0x014C` / optional `0x010B`, `/O2`, `spikes/view_matrix/d3d9.build.json`

Hashes differ from SOL’s published artifact (expected: MAIN build timestamp / non-reproducible link metadata). Source tree at `3929868`; code commits `d92a4e5` … `dddb35f`, doc-only tip `3929868`.

## Rollback checkpoint preserved

`deployment/deploy-backups/20261023-pre-sol-3929868/manifest.json`

| File | SHA256 |
|------|--------|
| `d3d9.dll` (`514bb03`) | `19DFDFB7C38751B4127A5C87AA32887F355546E172F20C7776D5CEC5F0B209BE` |
| `SingularityVR.ini` | `CBA10397FABF9CACB115D3809FF1B0B8A645C5D63FBE69CCB533050A126C1796` |

**SVTG unchanged:** `452EA0C0DDFB4721A7F9B9718629975CF021F9E079E1CD92235C076E9E376192`  
(`Binaries/vr-assets/hud/katorga/glove-animation/tmd_short_gauntlet.svtg`)

## L1/L2 regression gate (MAIN, 2026-10-23)

| Gate | Result |
|------|--------|
| `test-dual-tmd-gun-scale.ps1` (10k MinHook ABI) | PASS |
| `test_gun_scale_name_registry.py` | PASS |
| `test-dual-weapon-tmd.ps1` (dual-TMD, support gauntlet, holster, reload fixtures, WeaponLocker, OLED) | PASS |
| `test-tmd-short-gauntlet.ps1` | PASS |
| `test-tmd-short-gauntlet-d3d9` (D3D9 + D3D9Ex, Steam SVTG) | PASS |
| `test-knife.ps1` | PASS |
| `test-weapon-carry-ray` (4000 rays) | PASS |
| `test_carry_ray_wiring.py` | PASS |

No runtime code changes during this session.

## Steam INI deltas (minimal patch on accepted file)

Only these keys changed from the `514bb03` checkpoint INI:

```ini
[Input]
DualWeaponTmdRightWrist=1    ; added
HipKnifeIgnoreTMD=0           ; was 1

[Render]
; TmdShortGauntlet=1, TmdSupportGauntlet=1 already set

[Diagnostic]
DualWeaponTmdProbe=0          ; was 1 (noise reduction)
VolkCarryPoseProbe=1          ; added
```

**Preserved:** `VolkManualReload=1`, `ReloadNativeMagazine=1`, `ReloadPalmGrip=1`, Polish `[Localization]`, gameplay, gun align tables, Volk grip flags.

## PSVR2 short sequence (user)

Log path after session: `%LOCALAPPDATA%\SingularityVR\view_matrix.log` — archive full file before next launch.

### A — Weapon + right glove (P0/P1)

Volk S4 right, TMD left: L2 hold/release ×2, L2+R2 fire, rotate right controller.

**P0 mandatory receipts:**

- `dual-tmd gun-scale: ready reflected`
- `dual-tmd gun-scale: local gun_scale scale=0 -> 1`

If missing: capture owner/reflection witness and exact refusal — do **not** grade P0 PASS.

**P1:** `tmd-right-wrist final` during held L2 in **both** eyes with draw segments.

### B — Knife (P2)

Holster gun → right-hand knife → wrist rotate → swing/release. Note `knife-hand` / `DrawKnifeWithNativeGlove` / TMD cuff receipts.

### C — Volk magazine (P3)

Eject → shoulder mag → palm hold/rotate → insert → fire. Capture `volk-carry-pose`, wrist source, stage 12, replay. **Do not** tune magazine offsets.

### D — Two-handed regression

Left grip on Volk, rotate both hands, release. Require `tmd-support-gauntlet: ... support=1 eyes=2 tris=7110 ...` and support=0 release when possible.

## Physical grades (fill after PSVR2)

| Area | Grade | Notes |
|------|-------|-------|
| P0 gun visibility + gun-scale override | NOT TESTED | |
| P1 right wrist / final-eye matrices | NOT TESTED | |
| P2 knife alignment / render route | NOT TESTED | |
| P3 magazine placement / manual reload | NOT TESTED | |
| Regression two-handed TMD assembly | NOT TESTED | |

## Post-session

1. Copy log to `deployment/crash-evidence/20261023-sol-3929868-psvr2/view_matrix.log` + SHA256 manifest.
2. Update grades in this doc or `sol-3929868-psvr2-evidence.md`.
3. **Restore** checkpoint DLL/INI from `20261023-pre-sol-3929868` unless user keeps candidate.
