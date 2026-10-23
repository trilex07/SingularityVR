# bc3ab34 controlled PSVR2 protocol (MAIN)

**Installed DLL:** `081B278E8B5ECB267EB42F1AF0C5C92227270AED77C08CB0B37519B72C125DF4`  
**Commit:** `bc3ab3455d980ae4440d554da908b0e9fc2ba080`  
**Rollback:** `deployment/deploy-backups/20261023-pre-bc3ab34-segment-test/` → `19DFDFB7…` (`514bb03`)

After each segment, from repo root:

```powershell
.\deployment\bc3ab34-segment-playbook.ps1 -Segment A   # then B, then C
```

Restart Singularity through Steam between segments when INI changes.

## Segment A (active now)

```ini
DualWeaponTmdReady=1
DualWeaponTmdGunScale=1
DualWeaponTmdRightWrist=0
TmdSupportGauntlet=1
DualWeaponTmdProbe=0
```

Volk S4, TMD raised → L2 hold/release → L2 held + R2.

## Segment B

Set `DualWeaponTmdRightWrist=1`, restart, archive log as B, repeat L2/R2.

## Segment C

Set `HipKnifeIgnoreTMD=0`, restart, verify ini receipt. Knife + Volk manual reload smoke.

Reload flags already on: `VolkManualReload=1`, `ReloadNativeMagazine=1`, `ReloadPalmGrip=1`.
