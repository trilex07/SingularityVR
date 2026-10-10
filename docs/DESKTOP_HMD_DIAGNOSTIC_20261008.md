# Desktop-HMD diagnostic — 2026-10-08

The production DLL can now exercise the stereo/backbuffer path without a physical HMD or an OpenXR submit session. This is an opt-in diagnostic only; normal startup remains unchanged unless the two keys below are present in the first `[Diagnostic]` section of `SingularityVR.ini`:

```ini
[Diagnostic]
ForceDesktopVr=1
DesktopCapture=1
DesktopCaptureFrames=900
DesktopAutomation=1
```

`ForceDesktopVr` enables `SetVrMode(true)` when no XR session is available. `DesktopCapture` requests one readback after `DesktopCaptureFrames` Presents through the existing capture path. There is no fabricated controller pose, no haptic output, and no claim of optical headset validation.

`DesktopAutomation` is a diagnostic-only XInput sequence published through the same `XInputGetState` seam as the VR pad: A is held through the intro and menu, the left stick is held forward after the player pawn is available, and the right trigger is held in the final phase. It does not run in the production INI. The sequence is deliberately long enough for Steam/UE3 startup and logs explicit phase markers.

The runner supports the isolated candidates `desktop-hmd` and `desktop-hmd-shadow1`. `-LaunchViaSteam` uses `steam.exe -applaunch 42670`; `-RestartSteam` clears the client's stale “already running” state before launch. The runner saves the installed DLL/INI/VFX files and restores them in `finally`.

## Evidence

The VFX/stereo diagnostic run produced:

- log: `C:\Users\Trilex\AppData\Local\SingularityVR\view_matrix_2026-10-08_20-47-00.log`
- raw SBS capture: `C:\Users\Trilex\AppData\Local\SingularityVR\capture_2026-10-08_20-47-27.bmp`
- review image: `C:\Users\Trilex\vr-mods\deployment\visual-audit-20261008\desktop-hmd-capture-204727.jpg`

The review image contains two equal eye views at 5264x2684. The previous stretched right-eye quad is absent. The log contains `StartInVr: VR mode ON at startup (desktop HMD diagnostic; no XR submit)` and `capture: saved ... (5264x2684)`.

The shadow-path run used `VrShadows=1` and `ShadowMode=1`:

- log: `C:\Users\Trilex\AppData\Local\SingularityVR\view_matrix_2026-10-08_20-50-46.log`
- raw SBS capture: `C:\Users\Trilex\AppData\Local\SingularityVR\capture_2026-10-08_20-50-57.bmp`
- review image: `C:\Users\Trilex\vr-mods\deployment\visual-audit-20261008\desktop-hmd-shadow1-capture-2026-10-08_20-50-57.jpg`

This run proves the shadow branch is reached in stereo (`VR render: ... shadows=1`, `shadow sampling: ShadowMode=1`), with 920 shadow resources, zero shadow creation failures, and the same stable two-eye frame. The splash frame does not contain a receiver surface, so it cannot prove the final in-world shadow appearance; that still requires a gameplay capture with a known caster/receiver.

The Steam installation was restored after the test. Its current production DLL hash is `86FA0B0EFC1D62DA7E1303CF145DB985CE31EE3D43C954D7457CE141B4CCF930`.

## Gameplay automation evidence

The clean Steam run on 2026-10-08 used the x86 `/O2` diagnostic DLL (SHA-256 `8A1C52FFE568751CEE19D2181442A9DEC6265B3F68D6180909D348807FEAC81C`) and was restored automatically afterwards:

- log: `C:\Users\Trilex\AppData\Local\SingularityVR\view_matrix_2026-10-08_21-17-54.log`
- gameplay capture after movement: `C:\Users\Trilex\AppData\Local\SingularityVR\capture_2026-10-08_21-21-16.bmp`
- fire capture: `C:\Users\Trilex\AppData\Local\SingularityVR\capture_2026-10-08_21-21-43.bmp`
- review images: `C:\Users\Trilex\vr-mods\deployment\visual-audit-20261008\desktop-hmd-gameplay-start-212116.jpg` and `desktop-hmd-fire-212143.jpg`

The log records `desktop automation: phase=0`, acquisition of `RvPlayerPawnSP`, `weapon state: a weapon IS equipped`, `phase=1 (left stick forward)`, and `phase=2 (right trigger fire)`. During the fire phase the game receives `RT 255`, and the native script path records `StartFire`, `BeginFire`, and `FireAmmunition`; the diagnostic capture hook is requested at `FireAmmunition`. This validates desktop input routing and the muzzle-event seam, but it is not a substitute for a physical PSVR2 test of pose, haptics, or optical VFX placement.
