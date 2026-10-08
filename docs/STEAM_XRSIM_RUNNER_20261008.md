# Steam / OpenXR simulator runner

`tools/steam-xrsim-singularity.ps1` performs a complete desktop validation cycle for a staged SingularityVR candidate:

1. stops only the Singularity processes (and SteamVR processes when `-StopSteamVR` is requested);
2. records `d3d9.dll`, `SingularityVR.ini`, and optional `SingularityVfx.dll` in a timestamped rollback directory;
3. retries the copy after checking that the destination is free; if an in-use DLL allows rename, it atomically quarantines the old file, installs the candidate, and removes the quarantine; otherwise it reports `openfiles` evidence;
4. starts the installed Steam game binary with the 32-bit `dvr-xrsim` OpenXR manifest;
5. collects the newest `view_matrix_*.log`, simulator state, hashes, and readiness evidence into a JSON report;
6. restores the previous Steam installation by default.

The runner does not close arbitrary Windows handles or stop the Steam client. “Unlock” means closing the exact game processes, retrying the file operation, and using the safe rename-and-replace fallback when Windows permits it. If another process still owns the DLL and also denies rename, the runner stops with a truthful error and preserves the rollback directory.

From an elevated PowerShell prompt:

```powershell
Set-Location C:\Users\Trilex\vr-mods\SingularityVR-production
.\tools\steam-xrsim-singularity.ps1 -Candidate vfx -StopSteamVR
```

For the shadow candidate use `-Candidate shadow3`. `-SkipLaunch` performs only deployment and restoration checks; `-KeepCandidate` leaves the candidate installed for a later headset run. Reports are written to `deployment/visual-audit-20261008/steam-xrsim-run-*.json`.
