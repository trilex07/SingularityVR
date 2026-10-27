# PSVR2 shot resistance

Status: integrated behind an opt-in INI switch. The current source passes the x86 policy test and a fresh full x86 PE32 `/O2` DLL build; the x64 helper and authenticated dry IPC also passed. Physical feel remains unverified because the local SteamVR PSVR2 driver did not start in the current runtime checks.

## Behavior

`FireAmmunition` is the only shot event source. The game thread records the weapon fingerprint and timestamp under a small SRW lock. The render thread chooses the transient kick while its window is active, then returns to the idle wall. Kick, hold wall, and Off commands follow the configured `LeftHanded` weapon hand. Commands go through the non-blocking x86 named-pipe client, which deduplicates state changes.

| Weapon | Hold wall | Shot kick |
| --- | --- | --- |
| Pistol | Weapon 4,6,3 | Vibration 3,6,50 for 150 ms |
| Shotgun | Weapon 2,4,7 | Vibration 2,8,35 for 220 ms |
| Rifle | Weapon 3,5,5 | Vibration 3,7,40 for 120 ms |
| Sniper / railgun | Weapon 2,5,6 | Vibration 2,8,30 for 250 ms |
| Unknown / TMD | Off | Off |

The gatling mesh has no proven fingerprint and therefore remains Off. The profiles are provisional until checked on a headset.

## Opt-in test

Set `[PSVR2] AdaptiveTriggers=1` in the runtime INI, close the game, and start it through `tools/psvr2-bridge/run-session.ps1`. That script creates a per-session token file and starts the x64 server before launching Steam AppID 42670. `-Hardware` enables CAPI calls; without it, the server runs in dry mode. Adaptive triggers remain off unless OpenXR reports SteamVR, the session is focused, the game is in gameplay, and the server authenticated the pipe client.

Build the x64 server with `tools/psvr2-bridge/build.ps1`. The x86 DLL and x64 helper use the shared wire definition in `spikes/view_matrix/psvr2_ipc.h`. The helper loads the installed `psvr2_toolkit_capi_loader.dll` only in `-Hardware` mode. Each pipe disconnect clears both controller trigger effects.

## Evidence and limitations

- L1: fresh x86 `test-trigger-shot` build/run passes weapon mapping, kick profiles, timing window, fallback-to-wall behavior, and left-hand kick routing.
- L2: fresh production build is x86 PE32 `/O2`, SHA-256 `864844C7B3079172A4F44AAD987689557331FC6E87215EC96490279F776A8F95`; fresh helper build is x64 PE32+, SHA-256 `E9C4B78955DD557F4BD3830E4E62E6CEC6791B41249D6A2B7F20B7CED81835CD`.
- Current follow-up: the source contains the `FireAmmunition` event hook and per-frame `ShotRunner::Select`/client submission. A fresh x64 PE32+ helper build passed at `deployment/psvr2-bridge-validation-20261009/SingularityPsvr2Bridge.exe`, SHA-256 `177CFBE5E4D3C1A15D9D767D32B0223954DCE4978A5F8836BAE3EFFB3F89CDAD`. The rebuilt helper loaded the installed `psvr2_toolkit_capi_loader.dll` and resolved init/deinit/driver-active/set-trigger-effect exports in `--probe-capi` mode without initializing hardware.
- Current left-hand routing adjustment: `test-trigger-shot` passes with a left-controller kick assertion. Fresh full DLL build: x86 PE32 `/O2`, SHA-256 `8905B48112E36A6E8B7E07E3620BB9398F1683CA8FC5FA7FC2B67DCC1EF5C33C`; manifest records branch `fix/vr-menu-input-grip-performance`, HEAD `6c457edb836c4ec0c36134baa14c4547b63f5dd6`, and matching artifact hash. The install still has a different SHA, so the test bridge correctly blocks launch.
- IPC smoke: the default restricted runner returned `WaitNamedPipeW` error 2 while the bridge process stayed alive. Repeating the same dry x86-to-x64 smoke outside the workspace sandbox passed (`connects=1`, `sends=2`, `errors=0`); server stderr confirmed `dry bridge active; hardware calls disabled`. The report is `deployment/psvr2-bridge-validation-20261009/dry-ipc-smoke-report.json`. This verifies token-authenticated IPC in dry mode, not an in-game session or hardware effect.
- Runtime: the current DLL completed the Steam+xrsim route with `[PSVR2] AdaptiveTriggers` absent (default OFF), so no hardware effect was requested. A fresh authenticated in-game client/helper session has not been verified.
- L4: kick intensity, wall comfort, and return-to-wall feel need a PSVR2 Sense headset and active SteamVR PSVR2 driver.

The default is `AdaptiveTriggers=0`; ordinary gameplay and xrsim behavior do not change.
