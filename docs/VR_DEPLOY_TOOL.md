# SingularityVR deployment tool

`tools\deploy-singularityvr.ps1` reinstalls the current tested `d3d9.dll` and `SingularityVR.ini` after Steam has verified or replaced the game files. It creates a timestamped backup before changing anything, verifies SHA-256 after each copy, and checks that the DLL is PE32 x86. The `.cmd` wrapper is convenient for a normal Windows terminal.

After Steam finishes **Verify integrity of game files**, run from the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File .\SingularityVR-production\tools\deploy-singularityvr.ps1 -Action Status
powershell -ExecutionPolicy Bypass -File .\SingularityVR-production\tools\deploy-singularityvr.ps1 -Action Install -StopGame
```

If Windows reports that a file is in use, close Singularity first and rerun the command as Administrator. The tool only stops the exact `Singularity` process when `-StopGame` is provided; it does not stop Steam or SteamVR.

The install report is written under `deployment\install-reports`. Backups are under `deployment\install-backups\<timestamp>`. To restore one:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\deploy-singularityvr.ps1 -Action Rollback -BackupPath .\deployment\install-backups\<timestamp> -StopGame
```

An XInput DLL is optional because the current repository does not contain a canonical `xinput*.dll`. If the main build supplies one later, pass its path explicitly, for example `-XInputDllPath C:\path\xinput1_3.dll`; it will be backed up and copied using its original filename. Without that parameter the tool reports that no XInput file was copied and leaves any installed XInput file untouched.

Use `-KeepIni` only when deliberately preserving the INI that is already installed. The default installs the repository's current INI, including `ForceDesktopVr=0`, `DesktopCapture=0`, and `DesktopAutomation=0`.
