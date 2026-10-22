# Arm A — locker stability (`LockerStationGrantOrdinary=0`)

**Preflight (2026-10-22):**

- `d3d9.dll` SHA: `8B0A7EACDFAB159A9C661FC116BA4609A2C917864DED851519CF31AD2589D0E3`
- `LockerStationGrantOrdinary=0`, `LockerVirtualOrdinary=1`, `LockerVirtualApply=0`

**Play (same save as before, do not save after locker):**

1. Load gameplay → weapon station locker → interact (open UI, browse, close).
2. Open VR weapon wheel once after closing locker.

**Pass in log (`%LOCALAPPDATA%\SingularityVR\view_matrix.log`):**

- `ini: LockerStationGrantOrdinary=0` on attach
- `VR UI focus: … WeaponLocker` present
- **Must NOT appear:** `LoadEquipped grant count`, `LoadEquipped path mask`, `locker station grant: DISABLED` (grant flag is off)
- **Must NOT appear:** gameplay hook worst frame >>100ms on locker focus (subjective: no hitch in HMD)

**Pass in game:** no crash; locker usable; wheel shows owned weapons only.

---

## Result — 2026-10-22 ~11:20 (Steam log PASS)

**Attach:** L1 `view_matrix attached 2026-10-22 11:20:03`  
**INI:** L99 `LockerStationGrantOrdinary=0`

**Locker (×2):** L17736–17741, L21809–21814 — focus + Phase B plan only:

- `locker slots witness: count=2` (unchanged)
- `locker virtual: plan … mask=0x1 (phase B read-only)`
- **Absent:** `LoadEquipped grant`, `locker station grant`, `locker virtual: apply`

**Wheel:** L8801 `ownedMask=1`; after locker/play L24509 `ownedMask=3`; commits L21726, L24595, L24678.

**Stability:** log continues to L30625+ at ~85 fps; process exited cleanly (no crash line). Worst logged `gameplay hooks` spike this session L6218 **39.71 ms** (splash-era 112 ms at L456 is pre-gameplay).

**Arm A verdict:** **PASS** on crash/grant/hitch-in-log criteria. Four ordinary weapons on wheel **not** expected (owned inventory 1→3, not 0xF).
