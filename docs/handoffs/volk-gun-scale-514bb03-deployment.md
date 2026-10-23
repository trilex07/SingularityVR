# MAIN deploy: Volk gun_scale + TMD support gauntlet (`514bb03`)

**Role:** Physical validation candidate (not merged to `main`).

| Item | Value |
|------|--------|
| Branch | `fix/volk-impulse-palette-producer` |
| Handoff commit | `514bb03a0c7adb4e42261f6cdd593166370a540e` |
| Compiled source | `032ab8af4b61effa1f8ed0f18c5a30e02eccb420` |
| SOL DLL SHA256 | `7C473E25D295158AFDED3A06B2F11D07EF9C0F45DE98ABB15076AB2F15D0B991` |
| **MAIN x86 `/O2` installed** | `19DFDFB7C38751B4127A5C87AA32887F355546E172F20C7776D5CEC5F0B209BE` |
| Rollback | `deployment/deploy-backups/20261023-pre-gun-scale-514bb03/` → `BEA5A562…` (`cac4ce3`) |
| SVTG (unchanged) | `452EA0C0DDFB4721A7F9B9718629975CF021F9E079E1CD92235C076E9E376192` |

## Code review (summary)

- **`native_dual_tmd_gun_scale.h`:** fingerprinted detour on native `gun_scale` BoneScale getter; returns `1` instead of `0` only under dual-TMD ownership gates; refuses otherwise.
- **`tmd_support_gauntlet.h` + `d3d9.cpp`:** opt-in support path keeps SVTG 7110 + glove assembly on supporting wrist; legacy hide when OFF.

No geometry commits; no `9bb0e47`.

## Regressions (MAIN build)

| Suite | Result |
|-------|--------|
| `test-dual-tmd-gun-scale.ps1` | PASS |
| `test_impulse_compiled_abi.py` (built DLL) | PASS |
| `test-dual-weapon-tmd.ps1` (incl. support-gauntlet, volk probe, reload, WeaponLocker) | PASS (C++ + Python; PS1 exit noise only) |
| `test-tmd-short-gauntlet.ps1` | PASS |
| D3D9 + D3D9Ex 7110 | PASS |
| OLED | PASS |

## INI (Steam — surgical edits only)

```ini
[Input]
DualWeaponTmdReady=1
DualWeaponTmdGunScale=1

[Render]
TmdShortGauntlet=1
TmdSupportGauntlet=1
```

All other lines preserved (including existing `DualWeaponTmdProbe=1` from prior diagnostic).

## Runtime safety gate (automated smoke)

45 s direct `Singularity.exe` launch after deploy:

| Receipt | Seen |
|---------|------|
| `ini: DualWeaponTmdGunScale=1` | **Yes** (log ~103) |
| `tmd-support-gauntlet: enabled=1` | **Yes** (~222) |
| `dual-tmd gun-scale: ready reflected …` | **No** (menu / no pawn) |
| `dual-tmd gun-scale: local gun_scale scale=0 -> 1 …` | **No** |
| `dual-tmd gun-scale: refused …` | **Yes** — `local camera-owned pawn unavailable` (~2027) |

**Gate status:** **INCOMPLETE** until in-game Volk + raised TMD + L2. `ready` alone is insufficient; require **`scale=0 -> 1`** or a documented refusal with reason during Test A.

## PSVR2 acceptance (user)

### Test A — L2/R2 (Volk S4, TMD raised)

1. Weapon right, raise left TMD.
2. L2 press/hold/release ×2.
3. Hold L2, R2 once, release both.
4. Move/rotate right controller.

**Pass:** weapon visible both eyes during L2; R2 fires; tracking/recoil/reload OK; Impulse works; no duplicate/scale/freeze.

### Test B — Left Grip support

Acquire/release grip twice; rotate both hands.

**Pass:** glove + short sleeve + TMD hardware stay on supporting assembly; no stretch/float/duplicate; grip release restores independent TMD; power reservation unchanged.

**Log targets:** `tmd-support-gauntlet: … support=1 eyes=2 tris=7110 indexRestore=OK paletteRestore=OK` and support=0 transition.

### Test C — Smoke

Switch/holster, fire with TMD up, reload, stereo/shadows, Polish/OLED/locker brief check.

## Evidence branch (post-PSVR2)

After session: archive full `view_matrix.log`, grade A/B/C, push `handoff/volk-gun-scale-514bb03-psvr2-evidence`, restore `cac4ce3` unless user retains candidate.
