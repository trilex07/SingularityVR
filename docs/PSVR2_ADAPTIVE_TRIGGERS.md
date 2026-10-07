# SingularityVR PSVR2 adaptive triggers: bridge architecture

Research branch: `feature/psvr2-polish-parallel`. Prior work:
`spikes/view_matrix/psvr2_trigger.h` (profiles/gating/dedup/event API),
`tools/reference-tests/psvr2_trigger_probe.cpp` (x86-safe probe).

## Architecture

```
x86 d3d9.dll (psvr2_client.h: Submit on game thread, worker owns pipe)
  -> named pipe \\.\pipe\SingularityPsvr2_<pid>_<tag> (psvr2_ipc.h "SSV2" v1)
  -> SingularityVR.PSVR2Bridge.exe (x64, bridge_main.cpp + trigger_backend.h)
  -> PSVR2Toolkit CAPI (ToolkitBackend, only with PSVR2TK_AVAILABLE)
  -> PSVR2 Sense adaptive trigger (RIGHT hand)
```

Clean-room implementation adapted from KHARVOX (MIT, see
`docs/REFERENCE_VR_IMPLEMENTATIONS.md` + `THIRD-PARTY-NOTICES.txt`). One
fix vs KHARVOX: a SINGLE protocol version shared by game and bridge
(KHARVOX shipped launcher-C# v1 against C++ v3).

## Files

| File | Purpose |
|---|---|
| `spikes/view_matrix/psvr2_ipc.h` | Wire protocol: 20-byte header (magic `SSV2`, version 1, seq, u32 ms, length) + Hello/TriggerState(16B)/Shutdown; pipe/token validators; `ServerSession` (Hello-first auth, constant-time token compare, monotonic sequence, re-hello never consumes seq) |
| `spikes/view_matrix/psvr2_client.h` | x86 game-side `Client`: `Start` validates + spawns worker; `Submit` non-blocking generation bump; worker connects (250 ms poll), Hello-auths, forwards on change (50 ms poll), best-effort Off+Shutdown on `Stop` (bounded 5 s join); absent bridge = counted retries, gameplay unaffected |
| `spikes/view_matrix/psvr2_trigger_config.h` | `[PSVR2]` INI: `AdaptiveTriggers`, `TriggerPistol/Rifle/Shotgun/Sniper` triples + `TriggerRifleFire`; KHARVOX validation ranges; per-key fallback; `SelectWithProfiles` (proven equal to `Select` on defaults) |
| `spikes/view_matrix/psvr2_trigger.h` | `ProfileFor`/`Select`/`Pack` (tight 43-bit codec incl. Off reason)/`Runner` event API (`OnWeaponChanged/OnFirePressed/OnFireReleased/OnReloadStarted/OnReloadFinished/ClearTriggerEffects`)/`IBackend` seam |
| `tools/reference-tests/psvr2_bridge/bridge_main.cpp` | x64 helper: user-only pipe (`REJECT_REMOTE_CLIENTS`), `--pipe/--token/--log/--parent/--timeout-ms`, header-first framing, apply-on-change, clear-on-disconnect, Ctrl+C/parent-exit/shutdown handling |
| `tools/reference-tests/psvr2_bridge/trigger_backend.h` | `ITriggerBackend` + `DisabledBackend` (log-and-hold) + `ToolkitBackend` seam (declared only under `PSVR2TK_AVAILABLE`; body pending submodule + hardware) |
| `tools/reference-tests/psvr2_bridge/loopback_sender.cpp` | x86 sender: pistol→shotgun→rifle-fire sequence against a bridge; PASS iff delivered |

## Trigger profiles (initial, conservative)

| Weapon | Idle | Fire override | KHARVOX reference |
|---|---|---|---|
| OFF | — | — | `off()` |
| PISTOL | `weapon(4,6,3)` | same wall | `weapon(4,6,3)` light |
| RIFLE | `weapon(3,5,5)` | `vibration(3,7,40)` | identical |
| SHOTGUN | `weapon(2,4,7)` | same wall | `weapon(2,4,7)` heavy |
| SNIPER | `weapon(2,5,6)` provisional | same wall | none — NEEDS HEADSET VALIDATION |
| TMD/OTHER | Off | Off | fists/unknown → Off |

Gating (KHARVOX order): Disabled → UnsupportedRuntime → Unfocused →
!gameplay → ammo Empty blocks fire vibration only. INI example:
`[PSVR2]` / `AdaptiveTriggers=1` / `TriggerPistol=4,6,3` /
`TriggerRifle=3,5,5` / `TriggerRifleFire=3,7,40` /
`TriggerShotgun=2,4,7` / `TriggerSniper=2,5,6`.

## Safety rules (enforced)

Init once, persistent handles, no per-shot discovery; game thread never
blocks on IPC; any backend failure → Off, gameplay continues; bridge
clears on disconnect/shutdown/timeout/Ctrl+C; probe/sender never actuate
beyond explicit test commands; hardware test = MILD resistance, brief
hold, always clear, timeout, no game required.

## Verification status

- `test-psvr2-integration`: IPC codec/auth/replay/version rejection,
  config parse/fallback/defaults-parity, Pack uniqueness (11 commands),
  Runner events, no-bridge client — PASS, `/W4` clean.
- Loopback x86→x64 (no hardware): sender `connects=1 sends=4 errors=0`
  PASS; bridge log shows pistol wall → shotgun wall → rifle-fire vibration
  → Off → shutdown → clear → exit 0. Binary arch: sender x86, bridge x64
  (verify with `dumpbin /headers`).
- Hardware tested: NONE. Do NOT claim physical feel. Next: populate
  `tools/PSVR2Toolkit`, implement `ToolkitBackend`, run the mild-resistance
  hardware probe on a dev PC with a PSVR2 Sense controller present.
