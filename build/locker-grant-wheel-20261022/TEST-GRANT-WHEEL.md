# Station grant + weapon wheel (no LockerVirtualApply)

**Build:** `spikes/view_matrix/d3d9.dll`  
**SHA-256:** `4A7F6F32A84DD63FE920B2FABD5EC5A971BD8FBDB3E083D6A9515E75DDFEB5F9`

**Steam backup:** `deployment/install-backups/20261022-101500-grant-wheel`  
(previous d3d9: `133A8397…`, ini snapshot in same folder)

## INI (Steam `Binaries/SingularityVR.ini`)

```ini
[Input]
KeepAcquiredWeapons=1
WeaponWheelOwnedInventory=1
LockerStationGrantOrdinary=1
LockerVirtualOrdinary=1    ; optional plan logs
LockerVirtualApply=0       ; required OFF
```

## PSVR2 test

1. Load save on **station** with weapon locker.
2. **Open locker** (focus) — grant runs once per focus.
3. Close locker.
4. **Weapon wheel** — expect **4 ordinary sectors** (pistol, SMG, shotgun, sniper).

## Log (`%LOCALAPPDATA%\SingularityVR\view_matrix.log`)

- Startup: `ini: LockerStationGrantOrdinary=1`
- On locker focus: `locker station grant: pickup …` / `done granted=N mask 0x… -> 0xF`
- Wheel: `owned inventory mask=0xF` (or similar)

## Rollback

Copy `d3d9.dll` + `SingularityVR.ini` from `20261022-101500-grant-wheel` back to Steam `Binaries\`.
