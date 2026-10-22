# Locker station grant investigation — 2026-10-22

## Deployed session under test (Steam, user crash report)

| Item | Value |
|------|--------|
| **Installed `d3d9.dll` SHA-256** | `7EFFFD02E70F849702AAFB30131AE1164BEDFCDBCAB909E7300BB6F703135D15` |
| **Log attach** | `2026-10-22 11:02:18` (`%LOCALAPPDATA%\SingularityVR\view_matrix.log`) |
| **INI** | `LockerVirtualOrdinary=1`, `LockerVirtualApply=0`, **`LockerStationGrantOrdinary=1`**, `KeepAcquiredWeapons=1`, `WeaponWheelOwnedInventory=1` |

## Timeline (locker focus → grant)

| Step | Log evidence | Result |
|------|----------------|--------|
| Locker focus | L16593 `VR UI focus: … WeaponLocker` | Native 2-slot GFx locker |
| Slot witness | L16595–96 `mEquippedWeapons count=2 cap=6`, `mOriginalWeapons cap=2` | UI still 2-wide |
| TArray write (grant) | L17550 `locker virtual: apply station LoadEquipped grant count 2 -> 4 (cap=6)` | **GFx movie array mutated on focus** |
| Native refresh | L17633 `locker station grant: LoadEquipped path mask 0x1 -> 0x1 count=1 (wheel latch ON)` | **`LoadEquippedWeapons` invoked; owned inventory unchanged** |
| Post-grant log | L17634+ MDG spam continues | No AV string in log; user reports **crash** (likely after log flush or native re-entrancy) |

**Owned inventory before locker (same session):** L4219 `owned inventory: stage=ready mask=1 count=1` (pistol only).

## Per-weapon grant chain (failed build)

| Weapon | Archetype resolved | Native pickup | Pickup return | Inventory after refresh |
|--------|-------------------|---------------|---------------|-------------------------|
| ARPistol | (LoadEquipped path — no per-weapon pickup log) | Not used on this path | — | **mask still 0x1, count=1** |
| ARMachineGun | — | — | — | Not in inventory |
| ARShotgun | — | — | — | Not in inventory |
| ARSniperRifle | — | — | — | Not in inventory |

Prior sessions (not in this log tail) showed GObjects archetype scans failing for MG/shotgun/sniper when that path was enabled — consistent with **classes not loaded at station** and **full GObjects walk = hitch**.

## Wheel vs inventory

- Failed build OR’d **`stationMask` / wheel latch `0xF`** into draw plan when grant ran; with **`mask=0x1`** after refresh, wheel would still show **one owned sector** unless latch lied about ownership (selection would not equip missing weapons).
- **`LockerVirtualApply=0`**: virtual 4-slot plan was **log-only** except the grant’s TArray + `LoadEquipped` side effect.

## Hitch source (evidence)

| Candidate | Support |
|-----------|---------|
| Full **GObjects** archetype search | Historical frame budget spikes; user-perceived hitch on locker |
| **Focus-time** `mEquippedWeapons` rewrite + **`LoadEquippedWeapons` @ 0xF23840** | Log shows both on same focus; synchronous on UI/render thread; **inventory unchanged**; **crash** reported |

No additional timing hooks were added — existing logs already distinguish TArray write vs post-`LoadEquipped` refresh.

## Root cause (falsifiable)

**`LockerStationGrantOrdinary` focus hook mutates GFx locker state and calls native `LoadEquippedWeapons` without updating `NativeOwnedInventory`.** That path is high-risk (re-entrancy / invalid GFx state) and **does not achieve four ordinary weapons** on the wheel.

## Remediation in this branch

- **`LockerStationGrantOrdinaryTry`**: read-only witness when flag=1; **no TArray writes, no `LoadEquipped`, no GObjects grant, no wheel mask latch**.
- **`LockerStationOrdinaryWheelMask`**: always **0** (wheel uses **`ownedMask` only**).
- **Steam deploy (this MAIN run)**: build **`8B0A7EACDFAB159A9C661FC116BA4609A2C917864DED851519CF31AD2589D0E3`** with **`LockerStationGrantOrdinary=0`** (backup `deployment/install-backups/20261022-110721-locker-grant-withdraw`).

**Accepted Phase B baseline (user OK):** DLL **`1551C9F42CB66F0428DC97D69F670A3D89F6F71CE4A05E14206CF56E865267E9`** (`deployment/install-backups/20261022-095748`).

## A/B test plan (same save, do not persist experimental inventory)

| Arm | `LockerStationGrantOrdinary` | Expected |
|-----|------------------------------|----------|
| **A (deploy now)** | `0` | No locker hitch/crash; wheel = **actual owned** (likely 1 ordinary until proper pickup path) |
| **B** | `1` on withdraw build | At most **4× read-only** `inventory snapshot mask=…` lines; **no** TArray/`LoadEquipped` |

## Remaining uncertainties

- Exact **crash PC** (no fault in `view_matrix.log`).
- Whether **`LoadEquipped`** or **TArray count 2→4** corrupts GFx when `mOriginalWeapons` cap stays **2**.
- **Phase F**: grant four ordinaries via **`KeepAcquiredWeapons` native pickup (keep=1)** at **locker close** or station script hook — not implemented here.

## Acceptance status

| Criterion | Status |
|-----------|--------|
| No locker hitch | **Restore required** (withdraw deploy + grant OFF) |
| Four ordinaries on wheel, selectable | **Not met** — needs Phase F / pickup chain, not GFx TArray hack |
| Retention / stereo / input / hands | **Unchanged** in withdraw diff; **headset retest** after deploy |
