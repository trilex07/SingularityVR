# SingularityVR deployment tool

`tools\deploy-singularityvr.ps1` reinstalls the current tested `d3d9.dll`, `SingularityVR.ini`, the texture injector `dinput8.dll`, its system forwarder copy `dinput8_orig.dll`, `texture_override.ini`, and the prepared texture override set after Steam has verified or replaced the game files. It creates a timestamped backup before changing anything, verifies SHA-256 after each copy, and checks that both injected DLLs are PE32 x86. The `.cmd` wrapper is convenient for a normal Windows terminal.

After Steam finishes **Verify integrity of game files**, run from the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File .\SingularityVR-production\tools\deploy-singularityvr.ps1 -Action Status
powershell -ExecutionPolicy Bypass -File .\SingularityVR-production\tools\deploy-singularityvr.ps1 -Action Install -StopGame
```

The default texture set is variant `B` (IBP), matching the set previously tested with the injector. Use `-TextureVariant A` for the Lanczos candidate or `-SkipTextures` when only the DLL/config set should be restored. The assets are read from the sibling `SingularityVR-assets` and `SingularityVR-assets-local` worktrees; override them with `-AssetsRoot` and `-AssetsLocalRoot` if the worktrees move.

If Windows reports that a file is in use, close Singularity first and rerun the command as Administrator. The tool only stops the exact `Singularity` process when `-StopGame` is provided; it does not stop Steam or SteamVR.

The install report is written under `deployment\install-reports`. Backups are under `deployment\install-backups\<timestamp>`. To restore one:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\deploy-singularityvr.ps1 -Action Rollback -BackupPath .\deployment\install-backups\<timestamp> -StopGame
```

Use `-KeepIni` only when deliberately preserving the INI that is already installed. The default installs the repository's current INI, including `ForceDesktopVr=0`, `DesktopCapture=0`, and `DesktopAutomation=0`.
