# Locker virtual Phase B — PSVR2 test (2026-10-22)

**Installed:** manual deploy (profile guard still pins menu DLL `DF7A7FF2`; Side `C1665790` was live).

| File | SHA-256 |
|------|---------|
| **d3d9.dll** (now) | `1551C9F42CB66F0428DC97D69F670A3D89F6F71CE4A05E14206CF56E865267E9` |
| **SingularityVR.ini** (now) | `A6FB3B7FB243011CF3B1BDBD978086B93B9E433958966D7D8DB94644CB4A1751` |

**Rollback backup:** `deployment/install-backups/20261022-095748`  
(restores Side `C1665790` + prior INI `D4240271…`)

## INI toggles (this candidate)

- `[Input] LockerVirtualOrdinary=1` — phase B plan logs on WeaponLocker focus
- `[Diagnostic] MenuAssetWitness=1` — optional TArray bounds witness (4 lines max)
- `[Input] TmdNativeAim=1` — unchanged from Side test

## In-game (~5 min)

1. Menu smoke (attract OK).
2. Save with **≥2 ordinary guns** + heavy/grenade if possible.
3. **Station** → open **weapon wheel** (note which guns show).
4. Open **weapon locker** GFx → wait → close.
5. Wheel + special slots still OK.

**No expected UI change** (still not 4 native slots).

## Log

`%LOCALAPPDATA%\SingularityVR\view_matrix.log`

After locker open, expect:

- `ini: LockerVirtualOrdinary=1`
- `VR UI focus: … class=RvUIGFxMovie_WeaponLocker`
- `locker virtual: plan uiOrdinary=… controllerOrdinary=… mask=0x…`
- `locker virtual: ui[N] mesh=…`
- (optional) `locker slots witness: mEquippedWeapons … count=…`

Compare `mask=0x…` with earlier `owned inventory: … mask=…` / wheel contents.

## Rollback (PowerShell, game closed)

```powershell
$bin = "C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries"
$bak = "C:\Users\Trilex\vr-mods\SingularityVR-title-menu-fix\deployment\install-backups\20261022-095748"
Copy-Item "$bak\d3d9.dll" "$bin\d3d9.dll" -Force
Copy-Item "$bak\SingularityVR.ini" "$bin\SingularityVR.ini" -Force
```

Verify d3d9 hash returns to `C1665790…`.
