# MAIN deployment — hide knife after TMD (`702f0c3`)

**Branch:** `fix/sol-disable-knife-after-tmd`  
**HEAD:** `702f0c32adaa7e9d65df45da3f11ffafec9d89f6`  
**Base:** `a4a5214` (includes Valkyrie shoulder reach, optional two-hand accuracy)  
**Prior Steam candidate:** DLL `4B34CE06…` (`05a94ab` knife-route experiment)

## Policy change

Default **`[Input] HipKnifeWithTMD=0`**: after native **`mTMD`** ownership, cancel held knife/melee, block pickup, skip overlay + TMD knife joint, hide left-hip belt cue. Right holster and TMD powers unchanged. **`HipKnifeIgnoreTMD=1` no longer bypasses** this gate. `HipKnifeWithTMD=1` restores the old experimental path (not acceptance target).

See `docs/handoffs/sol-hide-knife-after-tmd.md`.

## MAIN L1/L2 (2026-10-23)

| Gate | Result |
|------|--------|
| `tools/knife/test-knife.ps1` | PASS (202 hip-knife checks + suite) |
| `tools/tests/test-dual-weapon-tmd.ps1 -SkipOled` | PASS |

## Build

| Artifact | SHA256 |
|----------|--------|
| SOL reference (SIDE) | `7ED6B837…72B6DC4E` (full hash from SIDE build log) |
| **MAIN rebuilt / installed** | `A61C3B534F5C4C40DC878EC68CC52A77E8599DB57C6272D85254E58B795F6E1C` |

x86 PE32 `/O2`, `d3d9.build.json` @ `702f0c3`.

## Rollback backup

`deployment/deploy-backups/20261023-pre-702f0c3/manifest.json` — restores **`4B34CE06…`** DLL + INI **`F5FD4613…`**.

## Steam INI (patched in place)

Added explicitly: **`HipKnifeWithTMD=0`**. Retained from prior candidate: `DualWeaponTmdReady/GunScale/RightWrist=1`, `VolkAuthoredGrip=1`, `TmdShortGauntlet` / `TmdSupportGauntlet`, Volk reload flags, Polish localization block.

## PSVR2 acceptance (user)

1. **Pre-TMD save:** knife pickup + sheath **once** (unchanged).
2. **After TMD acquired or load with owned TMD:** no left-belt knife cue; left-hip squeeze must **not** affect either hand; no knife pickup.
3. **Regression:** TMD raised, L2, R2 fire, **left Grip** two-hand — complete sleeve/support assembly intact.
4. **Optional:** Valkyrie shoulder mag + two-hand accuracy if exercised in this session.

Archive `%LOCALAPPDATA%\SingularityVR\view_matrix.log` after session; look for `ini: HipKnifeWithTMD=0` and absence of `hip knife: PICKED` post-TMD.

**Grades:** NOT TESTED until physical report.

Restore **`4B34CE06`** or **`514bb03`** after evidence unless user keeps build. No merge to `main`.
