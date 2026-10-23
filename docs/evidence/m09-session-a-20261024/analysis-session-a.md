# M09 Session A — corrected physical record and receipt analysis (2026-10-24)

Session window: `=== view_matrix attached 2026-10-24 00:28:33 - run starts here ===`
through final line (file ends 00:29:58). Archive SHA256 of the complete log:
`081DCA190A96F56662BB95EAB246C837755F632F3B8854C47990E8E95F577A4D`.
The live log truncates per session, so this file IS the complete Session A run
(10,445 lines). The game was not relaunched before this archive.

## Corrected grades (user report, clarified)

- Weapon firing feedback (pistol, Valkyrie, Volk): **PHYSICAL PASS**.
- Aiming without firing: no vibration — **expected behavior**.
  (The earlier "stationary FAIL" was withdrawn by the user: they were only
  aiming, never firing. No stationary-firing FAIL exists in any record.)
- Firing while holding the controller completely stationary: **NOT TESTED**.
  Scheduled inside the Session B session; no PASS may be claimed from this log.
- Normal TMD behavior: **PASS**. Input, two-hand grip, reload: **PASS**.

## Haptics receipt analysis (why zero receipts is expected, not instrumental loss)

- `[Haptics Trace] XINPUT RUMBLE` receipts: **0** — receipts cap at 16 and
  nothing was ever logged, so the caps are NOT the reason. The hook capture
  path (`Hook_XInputSetState`, `padEmu ON && idx==0`) never saw a single
  nonzero motor value in the whole session.
- Periodic xinput diagnostics prove the hook itself was alive:
  `xinput: 744 calls/sec (total 2802), packet advances/sec 0, SetState/sec
  2791 - port0 2791 ..., emulation ON` (lines 378–511 and windows after).
- `[Haptics Trace] SUBMIT/STOP`: **0** — consistent: `nativeAmp = raw/65535 = 0`
  every frame, and `TmdPulses=0` kept `overlay = 0`, so no
  `xrApplyHapticFeedback` was ever issued. Identity configuration
  (`Enabled=1 Level=1 TmdPulses=0`) confirmed in the log
  (`ini: Haptics Enabled=1 Level=1 TmdPulses=0 (defaults 1/1/0 ...)`).
- Comparator: the accepted M01 session log
  (`m01-bb234ba-psvr2-pass/view_matrix.log`, SHA `89D5479D…1CBE`) shows the
  SAME shape: 0 rumble receipts, `SetState/sec 2760 port0 ... emulation ON`.
  The game's authored XInput rumble route has never emitted nonzero values in
  any recorded VR session — this predates the M09 candidate.

## Where the felt firing feedback comes from

- `[Input] PSVR2 AdaptiveTriggers=1 pistol=1,5,8 rifle=3,5,5 rifleFire=3,7,40
  shotgun=2,4,7 sniper=2,5,6` is active in THIS session (accepted baseline
  configuration), runtime selected `SteamVR/OpenXR` ("adaptive PSVR2 triggers
  eligible"), and the trigger bridge spawned and connected:
  `PSVR2 trigger bridge spawned (PID 37772...)`, `PSVR2 trigger client
  connected asynchronously (pipe session configured)`.
- Weapon-fire events therefore produce PSVR2 Sense **adaptive-trigger**
  effects (incl. the dedicated `rifleFire` effect) through the bridge — this
  is the physical feedback the user reported and graded, identical in
  behavior to accepted M01/M03 sessions. No regressions were introduced by
  the M09 candidate.

## Consequences for Session B

- The instrumentation is ready: with `TmdPulses=1`, any native TMD power
  event produces `TMD EVENT` receipts (`start/stop accepted=1 armed=1`,
  capped 24, foreign TMDs rejected with bounded `TMD NON-LOCAL` receipts),
  and the per-frame loop logs `SUBMIT [n/32]` with the full gate tuple
  (allowed/tracked/poseValid/posValid/focus, xrResult) and `STOP [n/16]`
  on clear (pause/menu/cinematic/panel or session loss). Zero events
  remain a valid outcome when no power is used.
- Stationary-firing check in Session B: fire the Valkyrie/Volk/pistol while
  holding the controller motionless (steady aim). Expected instrumented
  result: adaptive-trigger firing effects persist (game-event driven), no
  OpenXR rumble submissions (raw stays 0). Grade the FEEL of that session
  separately: whether firing stationary without any effect would be a
  product decision — today the criterion is "no stuck/excessive vibration
  and no crash", plus documentation of the behavior.

Archived alongside: `installed.ini` (Session A effective INI,
`D2E91B0B…03D8`, no `[Haptics]` section), `d3d9.build.json` (candidate
provenance @ d97d066, dll `92D1BABC…C9C4`), and `install-report.json`
(the controlled 2026-10-24 00:23:49 M09 install report,
`deployment/install-reports/20261024-002349.json`).
