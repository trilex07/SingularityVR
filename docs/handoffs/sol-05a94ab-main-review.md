# MAIN pre-deployment review — `05a94ab` (`fix/sol-tmd-raised-weapon-knife`)

**Baseline:** physical `3929868` evidence; Steam checkpoint **`514bb03`**.  
**Candidate commits:** `9593169` (TMD right `BuildGunC` grip), `0cf6c17` (knife route ownership), `6e15e77` (Impulse/ADS/R2 diagnostics), doc `05a94ab`.

## 1. Mandatory review — **PASS (deploy authorized)**

| Requirement | Evidence |
|-------------|----------|
| TMD material-0 right glove uses authored **`BuildGunC`** target | `hand_anchor_math.h` `WeaponGripSection` includes TMD+dual; `PrepareIndependentLeft` sets `weaponSingleHand` via `VolkSingleGrip(..., weaponGripSection, ...)` and `BuildGunC(aimTarget)` + `AimTarget(weaponSingleHand, ...)` — same gate for no-TMD and TMD sections (`test-hand-anchor-math.cpp` proves no-TMD vs TMD delta parity). |
| **`VolkAuthoredGrip=1`** retained | Patched on installed INI (not template); was already `1`. |
| Two-hand support priority | `!supportGrip` guards on `VolkSingleGrip` / `TmdRightWristAnchor`; `supportGrip` branch unchanged (`tmdsupport::Target` / authored pair). `test-tmd-support-gauntlet` PASS. |
| **`gun_scale` producer** | `native_dual_tmd_gun_scale.h` unchanged ownership/refusal; witness now logs **`Impulse` / `ADS` / `R2`** separately (`g_rawImpulsePressed`, etc.). `test-dual-tmd-gun-scale.ps1` PASS (10k MinHook). |

### `knifehand::JointRoute::Native()` — focused regression **PASS**

`test-knife-hand-grip.cpp` `ClipTests()`:

1. Eligible pickup → native reserved frames 100–102, both eyes (`eyes&3==3`), no fallback.
2. Joint absent ≥2 frames → **`fallback=true`**, native returns false (standalone only).
3. New generation → native re-armed.
4. **`!eligible` → `Reset()`** (new pickup may re-arm native; standalone suppressed while `reserveTmd`).

Runtime coupling (`d3d9.cpp`):

- `DrawHipKnife`: **`reserveTmd`** suppresses standalone overlay when native route owns frame.
- `DrawKnifeWithNativeGlove`: per-eye once, **`tmdCuff=1`**, state guard restore; **`jointSubmitted`** blocks duplicate standalone same frame.

**Review note (not a block):** brief **`!eligible`** calls **`Reset()`** rather than latching fallback; CPU tests accept this. PSVR2 must confirm no dual-route in one pickup if tracking flickers.

**nativeCuff vs standalone:** When `reserveTmd`, early `return` before standalone log/draw; native cuff draw uses `g_nativeCuffRenderer` path inside glove draw, not parallel standalone `DrawHipKnife` indexed path.

### D3D9 fixture (MAIN environment)

- SOL desktop: HAL `CreateDevice` failed before path exercise — **blocked on SOL machine only**.
- **MAIN re-run:** `test-tmd-runtime-regressions.ps1` with Steam SVTG — **`PASS D3D9` + `PASS D3D9Ex`** (7110 tris, IB/palette restore).

## 2. L1/L2 regression gate (MAIN, 2026-10-23)

| Gate | Result |
|------|--------|
| `test-knife.ps1` (incl. hand-anchor + knife-hand-grip JointRoute) | PASS |
| `test-dual-tmd-gun-scale.ps1` | PASS |
| `test-dual-weapon-tmd.ps1 -SkipOled` | PASS |
| `test-tmd-short-gauntlet.ps1` | PASS |
| `test-tmd-runtime-regressions.ps1` (D3D9/D3D9Ex) | PASS |
| OLED (bundled in runtime regressions script) | PASS |

## 3. Steam deployment

**Backup:** `deployment/deploy-backups/20261023-pre-05a94ab/manifest.json`

| Artifact | SHA256 |
|----------|--------|
| SOL reference DLL | `018C132FDAB31BA7EC92A31EAF535DF2EFA6E2D57EFAB5B2E383611291DD8138` |
| **MAIN built / installed** | `4B34CE06F93DDB2387D75C5A4152BCD6AADD900ED3DBA9E6459F86A677BE496C` |
| Checkpoint `514bb03` DLL | `19DFDFB7C38751B4127A5C87AA32887F355546E172F20C7776D5CEC5F0B209BE` |

**INI deltas only:** `DualWeaponTmdRightWrist=1`, `HipKnifeIgnoreTMD=0`; retained `VolkAuthoredGrip=1`, Volk reload flags, Polish `[Localization]`, `TmdShortGauntlet` / `TmdSupportGauntlet`.

## 4. PSVR2 short session — **AWAITING**

### A — Weapon + right glove (P0/P1)

Volk S4 right, TMD left: raise TMD, L2 hold/release, **R2 while raised**, move right wrist.

**PASS requires:** aligned gun+glove, no R2 snap, Impulse/fire/recoil OK, stereo agree.

**Log:** `tmd-right-wrist final` with **`Impulse=1`** (not legacy misleading `L2=0` alone), **`Impulse`/`ADS`/`R2`** on gun-scale witnesses, both eyes.

### B — Knife (P2)

Holster → knife right → rotate/swing → release → **one reacquire**.

**PASS:** one knife, intact glove, **`knife-hand: reserve TMD cuff joint`** then **`native joint ... tmdCuff=1`** both eyes **OR** exclusive standalone — **never both same pickup/frame**.

### C — Regression

Volk mag, two-hand support, left glove/sleeve/TMD hardware, shadows.

## 5. Grades (fill after session)

| Area | Grade |
|------|-------|
| P0/P1 weapon + right glove | NOT TESTED |
| P2 knife route | NOT TESTED |
| P3 Volk magazine | NOT TESTED |
| Two-hand support | NOT TESTED |

Post-session: archive log, push evidence branch, restore **`514bb03`** unless user keeps candidate.
