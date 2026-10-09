# Shot-fired trigger resistance (PSVR2 Sense)

Branch `feature/trigger-shot` (worktree `SingularityVR-trigger`). Staged,
tested, NOT wired into `d3d9.cpp` yet. Base: production `7275802`.

## What it adds over the existing profiles

`psvr2_trigger.h` already gives each weapon a HOLD wall while aiming
(pistol 4,6,3 / rifle 3,5,5 / shotgun 2,4,7 / sniper 2,5,6, KHARVOX-validated).
`spikes/view_matrix/trigger_shot.h` adds the SHOT: a significant, short
vibration kick on every `FireAmmunition` event that decays back to the wall.

| weapon | hold wall | kick (pos, amp, freq, window) |
| --- | --- | --- |
| Pistol | Weapon 4,6,3 | Vibration 3,**6**,50, **150 ms** |
| Shotgun | Weapon 2,4,7 | Vibration 2,**8**,35, **220 ms** |
| Rifle (auto) | Weapon 3,5,5 + fire Vibration 3,7,40 | Vibration 3,**7**,40, **120 ms** |
| Sniper / railgun | Weapon 2,5,6 | Vibration 2,**8**,30, **250 ms** |
| Gatling | Rifle wall + spin-up 6/25→7/35→8/45→**8/55** by hold ms | (KHARVOX chaingun mirror) |
| TMD / unknown | Off | none |

All values inside the validated hardware ranges (KHARVOX
`validTriggerCommand`: Weapon start 2-7, end>start, end<=8, strength 1-8;
Vibration position<=9, amplitude 1-8, frequency>=1). Feel defaults need one
headset pass; the ini triples below tune without a rebuild.

## Files

| file | contents |
| --- | --- |
| `spikes/view_matrix/trigger_shot.h` | `WeaponFromGunVerts`, `KickFor`, `SpinUpKick`, `ShotRunner`, `ValidKickCommand` |
| `tools/reference-tests/trigger/trigger_shot_test.cpp` | test (PASS) |

Test: `PASS: shot-fired trigger resistance policy` (x86 `/O2 /std:c++17`).

## Integration (main agent)

1. Add one `triggershot::ShotRunner shotRun_;` next to the trigger Runner.
2. In the `FireAmmunition` block that already posts the VFX `OnFire` mailbox
   (same `FnIdx(kFnFireFirst)` gate): `shotRun_.OnShotFired(nowMs,
   triggershot::WeaponFromGunVerts(gunVerts))`. Gatling fingerprint is still
   `Other` until the census fills it — add the mesh value next to the GunAlign
   table when known.
3. Where the Runner's `Update()` runs per frame: `Command out =
   shotRun_.Select(baseWall, nowMs)` with `baseWall` from the existing
   `ProfileFor`/`Select`, then forward `out` (dedup by `Pack` already differs
   kick-vs-wall, so the backend sends on change only).
4. `fireDown` already exists (trigger action state); reload/ammo/menu gating
   stays in the base `Select`.
5. INI (extend `[PSVR2]`, parsed by `psvr2_trigger_config.h` conventions):
   `TriggerKickPistol=3,6,50,150`, `TriggerKickShotgun=2,8,35,220`,
   `TriggerKickRifle=3,7,40,120`, `TriggerKickSniper=2,8,30,250`
   (position,amplitude,frequency,windowMs).

## Bridge gap (honest)

The game side (x86) can only queue commands over the named pipe
(`psvr2_ipc.h`: `TriggerState`, client `psvr2_client.h` already exists). The
x64 helper that loads `psvr2_toolkit_capi_loader.dll` and calls
`SetTriggerEffect` does NOT exist in this repo — KHARVOX has it
(`Psvr2BridgeMain.cpp` + `Psvr2ToolkitBackend` + launcher session). Until that
helper ships, commands queue and degrade to null (gameplay unaffected, no
resistance felt). Build the x64 bridge before any headset feel test.
