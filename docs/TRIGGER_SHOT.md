# PSVR2 shot resistance

Status: integrated behind an opt-in INI switch; L1 policy test and x86/x64 builds pass. Physical feel remains unverified because the local SteamVR PSVR2 driver did not start in the current runtime checks.

## Behavior

`FireAmmunition` is the only shot event source. The game thread records the weapon fingerprint and timestamp under a small SRW lock. The render thread chooses the transient kick while its window is active, then returns to the idle wall. Commands go through the non-blocking x86 named-pipe client, which deduplicates state changes.

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

- L1: `test-trigger-shot` passes weapon mapping, kick profiles, timing window, and fallback-to-wall behavior.
- L2: production x86 DLL builds `/O2` PE32; helper builds x64 PE32+ and resolves the CAPI exports at runtime only when hardware mode is requested.
- Runtime: the game-side client and helper have not yet completed a live authenticated pipe session on this machine. The current xrsim replay deliberately cannot produce PSVR2 trigger effects.
- L4: kick intensity, wall comfort, and return-to-wall feel need a PSVR2 Sense headset and active SteamVR PSVR2 driver.

The default is `AdaptiveTriggers=0`; ordinary gameplay and xrsim behavior do not change.
