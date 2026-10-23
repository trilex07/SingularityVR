# M01 — Valkyrie TMD reload (integration `1c358e2`)

**Status:** **FAIL** (PSVR2 2026-10-23). Steam rolled back to `A61C3B53…`. See [`MAIN_M01_1c358e2_PSVR2_FAIL.md`](../../handoffs/MAIN_M01_1c358e2_PSVR2_FAIL.md).

## Source

| Field | Value |
| --- | --- |
| Requested commit | `1c358e2fd74d22af4c7fdfad0f4fb4382d83020d` |
| M01 code commit (ancestor) | `d897dc4` — ignore disabled TMD knife in Valkyrie shoulder gate |
| Branch (local) | `main-integration-m01` |
| Integration branch | `integration/sol-side-master-package-20261023` (HEAD `591d220` includes M03; M01 build intentionally at `1c358e2`) |

## Steam baseline (pre-deploy backup)

Backup: `deployment/deploy-backups/20261023-pre-m01-1c358e2/`

| File | SHA256 |
| --- | --- |
| `d3d9.dll` (702f0c3 hide-knife) | `A61C3B534F5C4C40DC878EC68CC52A77E8599DB57C6272D85254E58B795F6E1C` |
| `SingularityVR.ini` | `4EA3857A0E78033C162280AB1A8FCB8A5D6CA09B72547CE75C8D55688BC904EA` |

Preserved INI flags (unchanged): `HipKnifeWithTMD=0`, `ValkyrieManualReload=1`.

## MAIN build (2026-10-23)

| Field | Value |
| --- | --- |
| Command | `spikes/view_matrix/build.ps1` |
| PE machine | `0x014C` (332) |
| PE32 optional magic | `0x010B` (267) |
| Optimization | `/O2` x86 |
| **Deployed DLL SHA256** | `2218BCA833577B13B45B28E82DE9B54295EF02339DD52ED56937B8152D63035B` |
| SOL offline reference (881e18c worktree) | `E0CE640A99E30D80774916876514586FF2C8DA224FD2BF56DD62688557E35271` (different object; same source tree at M01 boundary) |

Deploy: **DLL only** (manual copy; INI not modified). No SIDE UI/Bink/audio/DDS/note assets.

## Offline regression (MAIN host)

| Check | Result | Notes |
| --- | --- | --- |
| `test-dual-weapon-tmd.ps1 -SkipOled` | **PASS** | 16× x86 + Python wiring; includes 30k next-weapon transactions |
| `test-dual-tmd-gun-scale.ps1` | **PASS** | |
| `test-right-eye-mirror.ps1` | **PASS** | D3D9Ex pixel/RT0 |
| `test-tmd-short-gauntlet.ps1` | **PASS** | |
| `test-tmd-short-gauntlet-d3d9.exe` (Steam SVTG) | **PASS** | HAL + D3D9Ex; 3304/3304 pixels both paths — **SOL HAL BLOCK resolved on MAIN** |
| `test-tmd-runtime-regressions.ps1` | **PARTIAL** | C++ through D3D9 gauntlet PASS; script aborts on `py -3` stderr wrapper (environment); not a game failure |
| `test-centurion.ps1` (default) | **PARTIAL** | Synthetic subtests PASS; runtime fixture **BLOCKED** without `-PreparedRoot` (private assets) |
| `validate_master_manifest.py` | **PASS** | Atlas SHA only; Bink/DDS dirs not supplied |

## PSVR2 protocol (user)

1. TMD owned; equip **Valkyrie**.
2. Circle → eject magazine.
3. **Grip before shoulder:** left Grip, then reach left shoulder → one magazine on left glove → insert → verify native loaded count → R2 fire.
4. **Grip at shoulder:** repeat with Grip only after inside shoulder volume.
5. **False pickup:** left Grip away from pouch/shoulder → must **not** spawn ammo.
6. Spot check: Volk reload, two-hand TMD assembly (L2/R2), no knife-on-belt regression (`HipKnifeWithTMD=0`).

After session: close game normally; copy full `%LOCALAPPDATA%\SingularityVR\view_matrix.log` here before relaunch.

**M01 PASS criteria:** both pickup orders, magazine follows left glove, insertion updates native ammo, firing works, no false pickups, no TMD/Volk regression.

## Rollback

Restore from `deployment/deploy-backups/20261023-pre-m01-1c358e2/` and verify:

- DLL `A61C3B534F5C4C40DC878EC68CC52A77E8599DB57C6272D85254E58B795F6E1C`
- INI `4EA3857A0E78033C162280AB1A8FCB8A5D6CA09B72547CE75C8D55688BC904EA`

## Log archive (post-PSVR2)

Place `view_matrix.log` in this folder. Grep targets for SOL:

- `valkyrie shoulder gate`
- native ammo / reload FSM receipts

**Do not start M03 (`591d220`) until M01 is graded PASS and M01 install is backed up.**
