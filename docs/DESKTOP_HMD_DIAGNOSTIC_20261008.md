# Desktop-HMD diagnostic — 2026-10-08

The production DLL can now exercise the stereo/backbuffer path without a physical HMD or an OpenXR submit session. This is an opt-in diagnostic only; normal startup remains unchanged unless the two keys below are present in the first `[Diagnostic]` section of `SingularityVR.ini`:

```ini
[Diagnostic]
ForceDesktopVr=1
DesktopCapture=1
```

`ForceDesktopVr` enables `SetVrMode(true)` when no XR session is available. `DesktopCapture` requests one readback after 180 Presents through the existing capture path. There is no fabricated controller pose, no haptic output, and no claim of optical headset validation.

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
