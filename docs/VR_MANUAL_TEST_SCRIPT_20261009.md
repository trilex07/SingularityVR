# Manual PSVR2 test and capture

## Start

1. Put on the headset and make sure PSVR2 is visible in SteamVR.
2. Launch with `tools/steam-vr-test.ps1`, or start SteamVR first and then launch Singularity normally from Steam. The VR launcher waits for `vrserver` and `vrcompositor` before opening the game.
3. Do not change SteamVR settings for this test.

If the SteamVR status window says **W trybie czuwania / Załóż gogle, by wybudzić VR**, the OpenXR runtime is idle. The game can still open a flat SBS window in that state; wake the headset first, wait until SteamVR shows it as active, and only then start Singularity.
3. In `VR Settings`, leave `RecenterChord` disabled unless the specific test is about recentering.
4. Turn **VR session recorder** on only after reaching the scene to test.

## L3 + R3 behavior

- A short L3+R3 chord requests one backbuffer screenshot in `%LOCALAPPDATA%\SingularityVR\capture_*.bmp`.
- With `RecenterChord=0` it does **not** recenter the view.
- It does **not** start or stop the recorder.
- The recorder is controlled separately from `VR Settings`; this prevents an accidental screenshot chord from losing the recording.

## Suggested recording route

1. Main menu: point at `VR SETTINGS`, open it with the laser, and verify the controller page.
2. Return to gameplay.
3. Walk forward and turn left to the first chest/fountain prompt.
4. Aim at a nearby lock and fire one shot.
5. Test left-hand movement, move it close to the weapon, hold the two-hand grip, then move it away.
6. Test holster and draw.
7. Pause, open `VR SETTINGS` with the laser, and return to gameplay.
8. Press L3+R3 once at each important state. Wait at least three seconds between captures.
9. Stop the recorder in `VR Settings` before quitting.

## Collecting the run

Run:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\collect-vr-test-evidence.ps1
```

Send the resulting `deployment\vr-user-test-*` directory. The manifest includes hashes for the BMP, log, and `.vrrec` files.
