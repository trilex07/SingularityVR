# Production baseline — origin/main

Date: 2026-10-11 18:14 +02:00. This audit does not change VR behavior. It records what `origin/main` actually builds and what that binary did in the installed Steam copy of Singularity.

Source commit: `f74210eb0a4f0ed3b7ddb52181f7448aa5f0ddaa` (`feat(vr): first playable version`, 2026-10-11 00:55 +02:00).

Worktree: `C:\Users\Trilex\vr-mods\SingularityVR-audit-baseline`, branch `audit/production-baseline`, created from `origin/main`. No other worktree was reset, checked out, or overwritten. The dirty `SingularityVR-production` tree on `fix/vr-menu-input-grip-performance` was left untouched.

## Tools that exist on this commit

`AGENTS.md` tells agents to use `tools/singularityvr-test-bridge.ps1` and `docs/VR_STATUS.md`. Neither file is in this commit. The simulator runner `tools/steam-xrsim-singularity.ps1` is present. It was not used. This run is the real Steam app, not dvr-xrsim.

Deployment and rollback used `tools/deploy-singularityvr.ps1`, which is present. The game was started with `steam.exe -applaunch 42670` after starting Steam and SteamVR. `XR_RUNTIME_JSON` and `DVR_XRSIM_DIR` were unset.

## Steam install before the candidate

| Item | Value |
|---|---|
| AppID | 42670 |
| Manifest `buildid` | 21300 |
| `Singularity.exe` | PE32 x86, 27,444,736 bytes, SHA-256 `BD459ACD87D4B82693928DEE4ED437C086D5928A8E0428F3AD54B6FC114C0F51`, version reported by Windows Error Reporting `1.1.0.49069` |
| OpenXR ActiveRuntime (HKLM) | `C:\Program Files (x86)\Steam\steamapps\common\SteamVR\steamxr_win64.json` |
| OpenXR ActiveRuntime (WOW6432Node) | `C:\Program Files (x86)\Steam\steamapps\common\SteamVR\steamxr_win32.json` |

Preserved at 2026-10-11 18:10:40 +02:00, before the candidate was copied, into `deployment/preserved-working-install/`. Copy hashes matched the live files. The deploy tool then made its own backup at `deployment/install-backups/20261011-181147` and restored from that backup after the crash.

| File | Size | SHA-256 |
|---|---:|---|
| `d3d9.dll` (working install) | 651,264 | `590788116C7C173737E285D6D1B9575544AD2517690003ECC24CE378FDD0BDAD` |
| `SingularityVR.ini` (working install) | 12,775 | `44A3C4DBE089C82D951916F772EB7EA6DB8849256CDA77EB22BA892FF9AEFD20` |
| `dinput8.dll` | 54,784 | `939E46948B60CC64A029E17480364A65C09E8FAEAA185DE6A33D15CF45EF21F8` |
| `dinput8_orig.dll` | 176,640 | `2A746CCF3922AECCED0387F3C6E21E55C6DF786DF8164C482F2A2E6165EDD395` |
| `texture_override.ini` | 1,164 | `2AE24B9C322988B28FDD103362D5D9121C54956009512123D12C0DFA01929D4B` |
| `SingularityVfx.dll` | 10,752 | `120E6C7534D2F3B29FE7DBD4D5464CDE179F14BD5A891608793D61A3ABCCF456` |
| `openxr_loader.dll` | 607,232 | `98B21D279402E84B95014D4F86259EFA75F3A015A715F71811150122D318C3F7` |

The working DLL is PE32 x86 (`machine 0x014C`, optional magic `0x010B`). It is not the binary produced from this commit. The repo INI at `spikes/view_matrix/SingularityVR.ini` is SHA-256 `2A0C3A5D8779067E171C7B30E1BE48FA10ACE0330E95F6C56367236C2DED64CE`. The install kept the working INI. Material drift: the installed file has `RecenterChord=1` where the repo file has `RecenterChord=0`, and the installed file also carries `CutsceneCinemaMask=1`, `MenuOverlay=1`, `VfxLook=0`, `ShadowTest=0`, `DropExtraTarget=0`, and `VfxMuzzleOrigin=1`.

Texture override DDS count at status time: 107. Those files were not replaced.

## Build of this commit

`spikes/view_matrix/build.ps1` with `OPENXR_SDK=C:\Users\Trilex\vr-mods\dependencies\openxr-local`. The script forces `/O2` for every production DLL. Independent check of the output:

| Check | Result |
|---|---|
| Path | `spikes/view_matrix/d3d9.dll` |
| SHA-256 | `A555AF4E7736357826CCC826EB6B8BE3F20313F09CFFEC8D465CDC1D510D6E91` |
| Size | 651,264 bytes |
| PE | x86 PE32, machine `0x014C`, optional magic `0x010B` |
| PE TimeDateStamp | `0x6ACBB514` |
| Manifest | `spikes/view_matrix/d3d9.build.json` records the same SHA, `/O2`, branch `audit/production-baseline`, commit `f74210eb0a4f0ed3b7ddb52181f7448aa5f0ddaa` |

`tools/deploy-singularityvr.ps1 -Action Install -DllOnly -StopGame` installed that DLL. Post-copy SHA-256 of `Binaries\d3d9.dll` was `A555AF4E7736357826CCC826EB6B8BE3F20313F09CFFEC8D465CDC1D510D6E91`. The INI hash stayed `44A3C4DBE089C82D951916F772EB7EA6DB8849256CDA77EB22BA892FF9AEFD20`.

## Steam test

Launch: Steam PID 5532, then SteamVR, then `steam.exe -applaunch 42670`. No simulator runtime. Singularity PID 37576 (`0x92C8`) started at 18:13:34. `view_matrix.log` attached at 18:13:35.

SteamVR did not find a headset. `vrmonitor.txt` records `VR_Init failed with Hmd Not Found (108)` and the status alert `Nie wykryto gogli (108)`, then `SteamVRSystemState_NotReady`. `vrcompositor` did not start. `vrserver` PID 17792 was up and logged `vrlink: ERROR: Null VTE_CLIENT_SESSION_ID`.

### Verified runtime facts

These come from the process, the Windows Application log, and `%LOCALAPPDATA%\SingularityVR\view_matrix.log` for this PID.

- The candidate DLL loaded. The log header is `=== view_matrix attached 2026-10-11 18:13:35 - run starts here ===`.
- Startup injected `-ResX=5264 -ResY=2684 -windowed` from the cached headset resolution of a previous run.
- The log says `no HMD`.
- `OpenXR ready` count: 0. `session running` count: 0.
- After 600 frames the log says: `StartInVr=1 but there is no OpenXR session after 600 frames - staying flat. Connect the headset and press BACKSPACE.`
- Seven `perf:` samples, about 27.2–28.8 fps (34.75–36.76 ms/frame). Each sample reports draws/frame peak 0, split 0, parallax 0, mono-fallback 0, and no `[TRUE STEREO ON]` suffix. The controller bucket is `pad ON , feed ON , controllers ON DESK` at about 28.3–28.5 fps, with about 99.6–100% of frames missed (worst 113 ms), wait 0.00 ms, submit 0.00 ms.
- Controller line, repeated: `controllers [MOTION FALLBACK - no touch bindings]: left on desk (0 um/frame), right on desk (0 um/frame), pad ON, XR input on, sync-fail 0`.
- Menu function names resolved in the log (`ShowMenu`, `OpenMenu`, `OpenMenuDelayed`, `IsAnyMenuOpen`, `IsMenuOpen`). That is a symbol resolve, not a displayed menu.
- INI echo includes `HudStereo=1`, `independent hands prototype=1`, `glove isolation=2`, `controller arms: gloves only`.
- Later in the same run the log records `hook hit #1: engine returned pitch=0 yaw=0 roll=0`, `IndependentHandsSuppressed verts=3314 first=0 prim=4830 type=4`, `GloveIsolation: suppressed detached sleeve/finger section verts=390 first=14490 tris=674`, polish text `AR9 Valkyrie`, and `RvPlayerPawnSP` / `SpawnDefaultPawnFor` process-event scans.
- The only `TRUE STEREO` string in the log is the hotkey help line `F1 TRUE stereo`. Stereo submission was not active.
- The process crashed at 18:14:15, about 40 seconds after attach. Application Error: `Singularity.exe` `1.1.0.49069`, faulting module `C:\Program Files (x86)\Steam\steamapps\common\Singularity\binaries\d3d9.dll` version `0.0.0.0`, timestamp `0x6acbb514` (matches this build), exception `0xc0000005`, offset `0x00043ca7`, PID `0x92C8`.
- No paired rerun of the preserved DLL was done. The crash is a property of this candidate in this no-headset launch. It is not an A/B against `59078811…`.

### Not physically verified

Nobody wore a headset for this run, and SteamVR reported no HMD. The following are not established:

- PSVR2 picture, comfort, IPD, world scale, or stereo appearance.
- Whether both eyes show different views.
- Controller tracking, button feel, or that a hand mesh follows a controller.
- That the glove-isolation log lines correspond to a visible glove.
- Menu layout, pointer, or readability in the headset.
- VR frame time. The 28 fps samples are flat-mode startup with no OpenXR submit.

## Rollback

The candidate regressed: it access-violated inside its own `d3d9.dll` during this launch. `deploy-singularityvr.ps1 -Action Rollback -BackupPath deployment\install-backups\20261011-181147 -StopGame` restored the previous DLL.

Post-rollback live hashes:

- `d3d9.dll` `590788116C7C173737E285D6D1B9575544AD2517690003ECC24CE378FDD0BDAD`
- `SingularityVR.ini` `44A3C4DBE089C82D951916F772EB7EA6DB8849256CDA77EB22BA892FF9AEFD20`

SteamVR processes started for this audit (`vrmonitor` 46200, `vrserver` 17792) were stopped by those PIDs. The Steam client was left running.

## Remaining problems

- `origin/main` built as x86 `/O2` (`A555AF4E…`) crashes in the real Steam game with `0xc0000005` at `d3d9.dll+0x43ca7` when no headset is present. The faulting timestamp matches this build.
- That same launch never created an OpenXR session. SteamVR state was HMD not found (108), so stereo, tracked controllers, and a headset menu were not exercised.
- The DLL that is installed and preserved (`59078811…`) is not a build of `f74210eb`. The installed INI does not match the INI in this commit.
- `AGENTS.md` still points at `tools/singularityvr-test-bridge.ps1` and `docs/VR_STATUS.md`, which are not in this tree.

## Physical verification still required

- A connected PSVR2 so SteamVR leaves `NotReady` and `vrcompositor` starts.
- A log from that session that contains `OpenXR ready` and `session running`, plus whether `[TRUE STEREO ON]` and non-zero split/parallax draws appear.
- Headset observation of the menu, both controllers, and the gloves. Log lines about glove isolation are not that observation.
- Whether `A555AF4E…` still crashes once a headset session exists. This run only shows the crash with no HMD.
- A comfort and frame-time check on the headset. The flat 28 fps samples do not answer that.
