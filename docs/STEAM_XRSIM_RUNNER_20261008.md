# Steam / OpenXR simulator runner

`tools/steam-xrsim-singularity.ps1` performs a complete desktop validation cycle for a staged SingularityVR candidate:

1. stops only the Singularity processes (and SteamVR processes when `-StopSteamVR` is requested);
2. records `d3d9.dll`, `SingularityVR.ini`, and optional `SingularityVfx.dll` in a timestamped rollback directory;
3. retries the copy after checking that the destination is free; if an in-use DLL allows rename, it atomically quarantines the old file, installs the candidate, and removes the quarantine; otherwise it reports `openfiles` evidence;
4. starts the installed Steam game binary with the 32-bit `dvr-xrsim` OpenXR manifest (use `-LaunchViaSteam -RestartSteam` to run through Steam and pass `-LogRoot` when the runtime logs are outside the shell's default LocalAppData);
5. collects the newest `view_matrix_*.log`, simulator state, hashes, and readiness evidence into a JSON report; simulator-state readiness requires a state-file timestamp from the current run and a PID matching the launched game, and both values are reported;
6. restores the previous Steam installation by default.

The runner does not close arbitrary Windows handles or stop the Steam client. “Unlock” means closing the exact game processes, retrying the file operation, and using the safe rename-and-replace fallback when Windows permits it. If another process still owns the DLL and also denies rename, the runner stops with a truthful error and preserves the rollback directory.

From an elevated PowerShell prompt:

```powershell
Set-Location C:\Users\Trilex\vr-mods\SingularityVR-production
.\tools\steam-xrsim-singularity.ps1 -Candidate vfx -StopSteamVR
```

For the shadow candidate use `-Candidate shadow3`. `-CandidateDirectory` accepts a reviewed candidate folder containing `d3d9.dll`, `SingularityVR.ini`, and optional `SingularityVfx.dll`; it avoids replacing another workstream's staged candidate. `-PostReadyHoldSeconds` keeps the actual game process alive for a concurrent deterministic replay. With `-LaunchViaSteam`, `-Windowed` forwards `-windowed -ResX=<width> -ResY=<height>` to AppID 42670 (default 1280×720) so menu inspection can use a visible desktop window. Without Steam, direct launch already uses those windowed dimensions. **Use this mode only for menu/UI navigation:** at 1280×720 the current run logged a mismatch against its 4224×2304 stereo scene/backbuffer and disabled stereo. The direct non-Steam windowed launch exited with code 0 after D3D9 device creation and before a fresh OpenXR session, so it was not a valid runtime run. Reports now preserve launch mode and requested resolution. `-SkipLaunch` performs only deployment and restoration checks; `-KeepCandidate` leaves the candidate installed for a later headset run. Reports are written to the selected report directory. xrsim is an installed-game integration check, not SteamVR or physical headset acceptance.
