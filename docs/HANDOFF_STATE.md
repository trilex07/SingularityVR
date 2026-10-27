# HANDOFF / SHARED STATE — SingularityVR production round, 2026-10-08

> Live doc. One section per workstream: OWNER, STATE, OPEN QUESTION + who answers it.
> Rule of this round: no code without a log line or screenshot demanding it.

## Deployed baseline (verified)

- Branch: `fix/vr-menu-input-grip-performance`, HEAD history includes: menu provenance,
  shadow stage-track + ShadowMode (0/1/2), height keep-on-transition, GunScalePct,
  hub (GUN SIZE, SHADOW, DROPEXTRA, MENU OVERLAY rows), Square-hold opens hub,
  cutscene-flat cherry-picks (d93e2db, c94d832), spike reporter (DEFAULT OFF),
  menu-draw census wiring, L3+R3 BMP capture, Fix-M station-menu FP hide (b623942,
  DEPLOYED 2026-10-08 ~13:46, sha8=8cb21258, BUILD==DEPLOYED: YES).
- Verify any DLL with: `code-tools\build_provenance.ps1 -Repo <prod> -BuildDll <dll>`
  (in SingularityVR-tools). `BUILD == DEPLOYED: YES` or it didn't happen.
- After EVERY build: grep the binary for a NEW string literal (comments don't
  compile in). Timestamp + size lie (stale-obj trap, twice).
- PowerShell 5.1 here: no `grep`/`sed`/`head`. Compile errors are Polish
  (`błąd C####`); scan builds for `błąd C[0-9]`, not `error C`.

## Workstream OWNERS (do not cross files without talking)

| Area | Owner | Notes |
|---|---|---|
| Production d3d9.cpp runtime | main agent (this branch) | small commits, provenance gate |
| Left-hand rendered wrist / palette / IK | other agent (hand/scale audit) | live IK UNSAFE until bone-slot map proven; data path proven by logs |
| Muzzle/smoke/blood VFX plugin | VFX agent (plugin) + main agent (HOST loader/ABI/draw, CODED+DEPLOYED, desktop PASS, plugin+atlases installed) | desktop PASS done, installed to Binaries |
| Cutscene flat/matte | cutscene agent (merged d93e2db+c94d832) | default ON; matte + flat share one gate; HUD paths do NOT honor flat (known flaw) |
| Polish localization | loc agent (feature/polish-localization) | draft 7056 (283 good); no game files touched; no .int files exist in install |
| Perf sampler ownership | main agent (tools: perf_compare.py) | sampler families all ≤0.7ms; game user-CPU ~9.5ms is the wall |

## MEASURED FACTS (log-backed, not opinions)

- Scale chain healthy: UU/m 52.5, eyeDelta 63-64mm (=user IPD 64), engine eye 70 UU
  (standard UE3 humanoid). WorldScalePct stays 100. "Too big" = eye height, see below.
- Eye height: runtime reports 0.55-0.58m for a standing 176cm user. User sits on
  floor for tests → reading is CORRECT, scale OK. Keep-position-on-transition
  implemented; sub-1.0m captures warn in log. Open: guardian floor check if it
  ever reads low while STANDING tall.
- Menu overlay = the GOOD menu (overlay OFF kills it, proven by user A/B).
- Menu suppression fires now (5331 draws/session, was 0) via stage 0-3 texture
  match. UNPROVEN whether it over-matches the menu backdrop (black-bg suspect #1).
- ShadowMode=2 drops sampling draws (21-221/window). UNPROVEN vs black background
  (suspect #2). User verdict on squares: modes change nothing; drop disables
  shadows (kept for now).
- Frame drops cluster at level transitions (58-914 draws + slow = engine
  load/streaming shape). Steady gameplay 85-90fps. Our hooks ≈0.6ms + residual.
- Spike reporter: DEFAULT OFF now (FrameSpikeReport=0). Re-enable per need.
- Black diamond quad (marked screenshots): NOT the provider overlay (survives its
  OFF). Prime suspect: diegetic in-world monitor mesh rendering black. NOT proven.
  NEW user report 08.10: diamond appears ONLY in station menus (UseStation/
  WeaponLocker, class 4) + world behind the station window slides with the RIGHT
  hand + a light matching the normal menu stays visible. Revised prime suspect:
  latched FP gun/arms rendering over the menu (engine-camera = centre-screen
  black diamond; ridden to controller = drags station view with hand).
  Fix-M coded + DEPLOYED on this branch (HideFpInMenu=1 default, counter g_vrMenuFpHidden):
  drops latched FP gun/arm indexed draws while ANY menu focused. Awaiting user run.
- ShadowMode=3 protect light-space CODED + DEPLOYED on this branch (pattern
  feature/shadow-mode3 @54dd4bd): generation draws native-once with census
  refresh (log `ShadowMode3 census`), light slots excluded from per-eye remap,
  hub SHADOW row extended 0-3 (`3 PROTECT LIGHT`), ini clamp 0-3, window report
  `ShadowMode3: N generation / M protected`. Default stays 1. Squares verdict
  pending user run.
- VFX host (Variant A) CODED + DEPLOYED on this branch (pattern
  feature/vfx-host-abi @702ce5c): Plugin/Drawer/Atlas seams, FireAmmunition
  entry -> OnFire mailbox (muzzle = GunAnchorWorld UU->m, dir = GunAimDir,
  weapon = Unknown -> stub alternates clips), Tick per Present (clamped dt),
  per-eye Collect(Gameplay)+DrawItems inside StereoPairBody with wrist-proven
  billboard basis, m->UU scaling via MetresToUU, HideMuzzleFx untouched (still
  default 1). OnHit NOT wired (no proven hit-point offsets - blood pending).
  Desktop tests PASS (host no-op/kill-switch, round-trip flash/smoke/blood).
  Plugin + 4 real DDS atlases INSTALLED to Binaries after desktop PASS.
  Round 2 fixes DEPLOYED (same branch): shadow census moved into the draw-hook
  shadow early-outs (StereoPairBody gate was dead - indexed/non-indexed shadow
  draws return before it; log-proven zero census lines with mode 3 live);
  VFX quads now get the live per-eye view*proj into fixed-function PROJECTION
  (stale engine transforms made them land outside clip = invisible).
  Round 3 DEPLOYED: shadow census asks the depth shader (bytecode, run-196
  method) which const regs it reads - old slot-scan read nothing (camMats are
  invalidated on shadow layouts; log-proven mask=0x0 at 480 draws/window);
  VFX reentrancy guard in all 4 draw hooks + magenta-dot/NDC diagnostic
  (8 dots) to split position-vs-texture blame.
  Round 4 DEPLOYED: atlas textures bypass the pool translation via
  g_origCreateTexture (prompt-atlas precedent; per-mip LockRect upload suspect);
  magenta dot now persistent while effects live (8-dot limit was ~4 frames -
  unmissable-by-design, user saw none).
- White ring right-eye-only (12:54:08 shot): per-eye sprite artifact, NEW instance
  of the muzzle-on-seam class. SpriteQuadPerEye=1 set in user ini; verdict pending.
- Menu-draw census (MDG groups) flows since 315ce56. 470 groups/session typical.
- L3+R3 = screenshot BMP next to log (+also recentres, pre-existing). Debounced 3s.

## Workstream: shadow-floor artifact — `ExtraCamReg` RETRACTED, cause still open

- OWNER: shadow agent (side worktree `SingularityVR-shadow-fix`, branch
  `feature/shadow-artifact-fix` @ `bbcc83c`). **No production source touched.**
- STATE: the one-eye banded/hatched shadow-floor artifact is now CAPTURED (29 SBS
  frames, `review-screens\`; eye alternates between frames). `fp_shadow.h`
  SPECULATIVE -> CAPTURED in the side worktree. **The `ExtraCamReg=13` proposal is
  WITHDRAWN — the conflict flagged in the previous revision of this entry is now
  resolved AGAINST it.** `SingularityVR.ini.example` (committed) records run 43
  having already tried 13: *"geometry stretched to the edge of the screen AND the
  shadows were unchanged ... c13 is NOT the carrier for the mis-projected shadow
  geometry, so the shadow's cause is elsewhere."* The handoff's "never enabled"
  claim was checked against INIs and the runtime log only — both genuinely `-1` —
  and missed `.example`, which is where trial history lives. Direct experiment beats
  inference: run 43 stands. Deployed INI returned to `ExtraCamReg=-1`.
  Full evidence + correction: `docs\HANDOFF_EXTRACAMREG_SHADOW_20261009.md`.
- WHY the c13 line of reasoning failed: the supporting facts were all real (scan
  logged `** c13 ROW shape=ok hits=2 zero at camActor`; only scan-locked c0 is
  remapped; `mono-fallback=0` in 129/134 gameplay windows) but they established
  *c13 is an unremapped camera matrix*, not *c13 carries this artifact*. Run 43 had
  already falsified the second step by experiment.
- Rejected, do not re-trove: **`ExtraCamReg=13` (run 43, tried + failed)**;
  ShadowMode=3 census is a provable no-op (matches the
  `mask=0x0 at 480 draws/window` already logged above); SplitColourTargetsOnly
  excludes nothing (whitelist admits both scene-sized A16B16G16R16F) which is also
  why g_dumpShadowConsts is unreachable; excluding the third target alone only moves
  the artifact to both eyes (run 41).
- ⚠️ **INI COLLISION:** deployments overwrite `Binaries\SingularityVR.ini`, which
  has now cost two keys today (`GloveIsolate=2` wiped at 18:58:55, and `ExtraCamReg`
  rewritten both ways). Read-once-at-DLL-load, so editing while running is inert.
  Check before EVERY test launch:
  `Select-String -Path "<Binaries>\SingularityVR.ini" -Pattern "ExtraCamReg|GloveIsolate"`.
  **The durable fix is to hold intent in the SOURCE INI**, not the deployed copy.
- REVERT / current correct value: `ExtraCamReg=-1`.

## Workstream: gloves-only hands + weapon-only hip holster — latest runtime route

- OWNER: main agent. User explicitly authorized continuing P2 while leaving
  performance work paused.
- STATE: the source-level independent glove and weapon-only holster policies
  remain implemented but visually unverified. The user's acceptance target is
  separately tracked hands, gloves only (no arm flesh), and a holstered weapon
  that moves independently of both hands.
- 2026-10-09 22:51 Steam+xrsim visible-window hold: runner deployed candidate
  `47265B53…3C5D1` through Steam with `-windowed -ResX=1280 -ResY=720`; the game
  stayed alive for the 120-second hold. xrsim state belonged to the live game
  PID, was FOCUSED, had two projection views, 8,292 submits, and zero errors.
  The runtime log explicitly reported that the 4224×2304 stereo scene target
  did not match the 1280×720 window/backbuffer and stereo was inactive. The
  route remained menu-focused: no equipped-hand draw, glove census, palette
  write, holster store/draw event, or fresh capture. CUA exposed no controllable
  app/window surface during the run. This validates a live simulator session,
  not rendered hands, holster behavior, or stereo pixels. The runner restored
  the original DLL/INI/VFX; post-run hashes match baseline.
- 2026-10-09 22:48 direct windowed attempt: the process exited with code 0
  shortly after D3D9 creation without a fresh OpenXR session. Its earlier
  `runtime_ready` report was a false positive because the runner read stale
  `state.json` from another PID; do not treat that attempt as runtime evidence.
  `tools/steam-xrsim-singularity.ps1` now requires a matching game PID and a
  state file updated during the current run before state JSON can establish
  readiness, and reports state PID/freshness explicitly.
- 2026-10-09 22:58–23:03 visible xrsim captures at 4224×2304 now show the actual
  main menu in two nonblack eye images (`Start Game`, `Quit Game`) with both
  controller rays visible. The 22:58 run passed all eight provenance gates;
  report recorded `runtime_ready=true`, matching live PID/state PID 32608,
  `runtime_state_fresh=true`, two projection views and zero errors. The capture
  had one projection layer, 61.66% nonblack in each eye, and no mono/stale
  submits. It is fresh visual evidence of the menu/stereo compositor route, not
  evidence of hands in gameplay or holstering. Artifact:
  `deployment/agent-test-bridge/gloves-only-holster-47265-20261009/captures/p2-stereo-4224x2304_sbs.png`.
- 2026-10-09 23:01 follow-up: startup showed the Raven splash before returning
  to the same main menu. `hand r aimtrim -17 -17` was acknowledged by xrsim, but
  the captured ray endpoint did not visibly move; the final direct grip-rotation
  command timed out as the 90 s runner hold ended, so it produced no result.
  Source shows the menu ray is computed from the right-hand aim orientation and
  the native UI click is driven by the controller trigger. The next route should
  test an explicit right grip pose, inspect the fresh ray, and only then send a
  trigger edge. Do not infer cursor placement from the prior A-button probe.
- The two runs restored the previous Steam DLL/INI/VFX hashes, and no Singularity
  process remains. The 22:58 report's SHA values match the pre-test baseline.
- NEXT: use the visible windowed mode only to reach or inspect menus. For hand,
  holster, or stereo evidence, use matching scene/backbuffer dimensions and a
  live equipped-weapon scene, then capture fresh stereo pixels while moving
  one controller at a time and storing/drawing the weapon.
- 2026-10-10 00:14–00:22 current owner follow-up: Steam now has candidate
  `A9D0B9AB…E4B66D7A` installed with `-KeepCandidate`; the mandatory preflight
  passed all eight checks (x86 PE32 `/O2`, built/deployed SHA equal, manifest
  matches HEAD). Live PID 11040 was paired with xrsim state `FOCUSED`, two
  projection views, zero errors. Fresh 4224×2304 SBS capture
  `../../xrsim-scratch-run/capture/a9-after-enter_sbs.png` shows the title menu
  (`Start Game` / `Quit Game`), not an equipped-weapon scene. Therefore this
  run proves startup/stereo integration only; it does not retest gloves or the
  holster. Keep `IndependentHandsAudit=1` and `HolsterAudit=1` only in the
  staged test INI. The active Steam INI has those diagnostics enabled for this
  live test; production source INI remains diagnostics-off. Latest collected
  bridge evidence is `deployment/agent-test-bridge/owner-audit-20261010-002156/`.
- NEXT: continue only after the menu route reaches the existing save; capture
  both hands independently, then store/draw the pistol and verify that the
  holster carries only the weapon. Do not promote this startup-only run as P2
  acceptance.

## OPEN QUESTIONS → who answers, with what test (all ≤1 session, no code first)

1. Black menu background: drop or suppression? USER: at black menu, hub SHADOW
   2→1→0 live. Returns on 1/0 = drop did it (then: gate drop outside menus).
   Stays black = suppression over-match (then: narrow match, counter proves it).
2. Transition drops: transient load or stuck? USER: do drops recover after ~15s in
   menu, or persist? Log ends healthy last time (=transient).
3. Black diamond identity: USER: hand still, move HEAD — diamond world-fixed or
   following? + which menu (pause/dialog/main) + L3+R3 burst staring at it.
   SUPERSEDED by Fix-M build on this branch: open ANY station menu (upgrade/
   weapon locker) with the new DLL — diamond gone + window world stable = fix
   confirmed (log line `Fix-M/menu-station` proves the counter). Diamond still
   there = diegetic-monitor theory back on, L3+R3 burst staring at it.
4. Ring artifact: USER: SpriteQuadPerEye=1 verdict (both eyes now?).
5. Height: USER: guardian floor + recentre standing tall (only if 0.58 returns
   while standing).
6. Shadow floor bands — **RECONCILED 09.10, the conflict is resolved AGAINST
   `ExtraCamReg`.** `SingularityVR.ini.example` (committed) records run 43 having
   already tried 13 with the result *"geometry stretched to the edge of the screen
   AND the shadows were unchanged ... c13 is NOT the carrier."* The handoff's
   "never enabled" claim checked INIs + runtime log (both genuinely `-1`) and missed
   `.example`, which is where trial history is kept. **Do not re-arm 13** — it is a
   known failure with a visible side effect; deployed INI is back to `-1`.
   The artifact itself is still OPEN (run 43 only ruled out c13). Next real lead is
   whichever of §4's blockers gets unblocked first, not a repeat of run 43.
   Owner reconciliation: the production source INI is now `ExtraCamReg=-1`, matching
   the run-43 result; the installed INI was verified at `-1` after DLL-only validation.
7. Gloves-only model (updated 09.10): the old 19:09 log did load `GloveIsolate=0`,
   but that is no longer the source configuration. Production source INI now has
   `ShowControllerArms=0`, `GloveIsolate=2`, `IndependentHandsPrototype=1`,
   `ArmIKMode=0`; the Steam+xrsim trial loaded those values. Fresh L1 glove/finger,
   reference-integration and holster tests pass. The 14:18 recording produced no
   `GloveCensus`, successful independent palette-write window, holster event, or
   fresh visual capture. Its source has 112 left-grip samples and zero right-grip
   samples; the historical `WeaponHolstered` flag is not replayed as a game command,
   so it cannot test the right-grip toggle. A sanitized 2,800-sample derivative now
   isolates left-only then right-only hand motion between right-grip store/draw
   pulses; offline replay passes and the original `.vrrec` is unchanged. Settings
   and L1 policy tests pass; rendered gloves and weapon-at-hip remain unverified
   pending an equipped-weapon gameplay state. Do not stack a shadow variable. Detail:
   `docs\HANDOFF_GLOVEISOLATE_20261009.md`.

## Workstream: independent glove rendering and weapon-only hip holster

- OWNER: main agent (`spikes/view_matrix/d3d9.cpp`, `spikes/view_matrix/SingularityVR.ini`).
- STATE: source config selects gloves-only (`ShowControllerArms=0`, `GloveIsolate=2`),
  enables the independent-hands prototype, and leaves arm IK at mode 0. Holster is
  enabled by its code default with source offsets `8,12,-30` UU and radius `12` UU.
  During holster, `FgRidesGun` leaves recognized arm/glove draws on their own pose;
  the latched gun alone follows `HolsterHipWorld`. `ReportFgRide` reports successful
  independent palette writes only for nonempty one-second windows.
- EVIDENCE: fresh x86 L1 tests passed: `glove_finger_test` (glove isolation and
  finger posing), `test-reference-integration` (fake XR pose conversion, arm
  palette, independent left/right section routing, and the new distinct-target
  invariant proving a left-target change leaves the right delta unchanged and
  vice versa), `test-weapon-holster` (radius, rising-edge toggle, hold, re-entry
  and gates). Candidate x86 PE32 `/O2` SHA-256
  `47265B530E2DB21DED35A5BC8B94E3C92CD7B2F166A68DD6F93AE9546873C5D1`; manifest
  matches HEAD `6c457edb`. Steam+xrsim loaded the intended settings and replayed
  the original route (1,420/1,420 batches) and the new isolated route (702/702
  batches, 38.0 s; FOCUSED, two projection views, 2,305 submits, zero runtime
  errors). The second run loaded the intended settings and produced valid left
  and right controller poses, but remained menu-focused and outside an equipped
  hand draw: no glove census, successful palette-write window, holster store/draw
  event, or fresh screenshot. Collected BMPs are older (17:50 and 18:23). The
  runner restored installed DLL, INI and VFX; post-run hashes confirm it.
  Source recording audit: 5,570/5,573 samples carry the historical gameplay bit,
  weapon field is 5799 throughout, 112 left-grip samples, zero right-grip samples.
  The new `hands-holster-isolation-route.vrrec` validates offline (2,800 samples;
  no trigger, stick, UI or recenter actions; two right-grip rising edges; isolated
  left/right motion phases).
- NEXT: run that route in an equipped-weapon gameplay state. Verify both gloves track
  independent controller poses, no animated forearm mesh appears, then store and
  redraw the weapon while confirming it alone stays at the hip. Capture current
  rendered pixels. The recording is a route reference, not an acceptance gate. The
  user authorized loading the current save for a brief test, but this run did not
  leave the menu; continue only after the live menu is visibly identified.
  Artifacts: `deployment/agent-test-bridge/gloves-only-holster-47265-20261009/reports/steam-xrsim-run-20261009-222553.json`,
  `hands-holster-replay.json`, and collected runtime evidence at
  `../../deployment/agent-test-bridge/20261009-222846/`. A second launch was
  stopped before UI input because CUA exposed no app windows; Ctrl+C interrupted
  the runner before its `finally` restore. The exact `runner-backup-20261009-223023`
  was then restored manually, with DLL/INI/VFX hashes verified against the first
  run's baseline. No menu selection or save interaction was made.
  Follow-up: `hands-holster-gameplay-route-v2.vrrec` added one A/UI-select edge,
  a 15 s stationary window and preserved both holster pulses and separate hand
  phases. It applied 704 batches in 58.0 s with two views and zero runtime errors,
  but the log continued to report `menu-source ... while menu focused`; there was
  no `GloveCensus`, palette write, holster store/draw, or capture. Its final
  `recenter` only recentered xrsim LOCAL and did not capture game pixels. The
  original generated route had a triangular timestamp bug and was stopped at
  sample 831; the corrected v2 passed offline and completed runtime replay.
  The v2 runner/report and collected log are
  `deployment/agent-test-bridge/gloves-only-holster-47265-20261009/reports/steam-xrsim-run-20261009-223845.json`,
  `.../hands-holster-gameplay-replay-v2.json`, and
  `../../deployment/agent-test-bridge/20261009-224031/`.
  CUA reported `apps: []`, so the menu could not be inspected or navigated. Next
  run must start with the current save already visibly in gameplay; user consent
  to load that save remains in effect.

- 2026-10-09 23:14–23:17 visible Steam+xrsim follow-up: candidate preflight passed
  all eight gates, state PID 20032 was fresh, FOCUSED, two projection views, zero
  errors; the 4224×2304 game window was visible. I aligned the right-hand ray to
  `Start Game`, inspected the next screen, then selected its highlighted `Continue
  Game` entry. The existing save loaded to a scene with `ARPistol` equipped; no
  `New Game` choice was made. Save `Player0.ue3profile` remained 1,885 bytes with
  its prior timestamp. Fresh captures are in
  `deployment/agent-test-bridge/gloves-only-holster-47265-20261009/captures/`:
  `p2-after-start-click_sbs_sbs.png`, `p2-latest-current-save-gameplay_sbs_sbs.png`,
  and the hand/hip comparisons. Moving only the left controller by 25 cm produced
  no visible independent glove and no `GloveCensus` or successful independent-
  palette window. Moving the right controller to the configured hip point moved
  the pistol out of view before the grip input, so that is not evidence of
  holstering. A right-grip edge then produced no `weapon holster: STORED/DRAWN`
  log and no weapon visible at the hip. Both runtime features therefore remain
  unverified; rendered hands are still broken. The runner restored all three
  installed files, verified against original hashes, and stopped the game.
  Collection: `../../deployment/agent-test-bridge/20261009-231750/`; report:
  `../../deployment/visual-audit-20261008/steam-xrsim-run-20261009-231428.json`.
  NEXT: add bounded diagnostics for hand-palette eligibility and the grip edge,
  then repeat the visible current-save route before changing mesh classification.
  Latest startup-only follow-up (22:44:51–22:47:13): the same manifest-matched
  candidate passed the mandatory post-deploy preflight (all eight checks true),
  launched through Steam, and remained alive for a 120 s runner hold. xrsim state
  was FOCUSED with 2 projection views, 8,306 submits, zero mono/stale submits and
  zero errors. No route or controller input was sent: all controls remained neutral,
  the game log stayed `menu-source ... while menu focused`, and there was no
  `GloveCensus`, successful independent-palette window, holster event, or capture
  (`captureSeq=0`). The collector copied only stale BMPs from 17:50 and 18:23. CUA
  still returned `apps: []` while the game was active and `cua.listWindows` was
  unavailable, so the current save was not loaded and no save state changed. The
  runner stopped PID 36528 and restored the baseline successfully. Report:
  `deployment/agent-test-bridge/gloves-only-holster-47265-20261009/reports/steam-xrsim-run-20261009-224451.json`;
  collected runtime evidence: `../../deployment/agent-test-bridge/20261009-224712/`.

## Workstream: shot-fired PSVR2 trigger resistance

- OWNER: main agent for production wiring; trigger policy originated on
  `feature/trigger-shot` @ `ca105a8`.
- STATE: integrated in the current uncommitted production worktree. `FireAmmunition`
  records the shot/weapon into `triggershot::ShotRunner`; `UpdateAdaptiveTriggers`
  selects the per-frame kick/hold command and submits it through the asynchronous
  x86 client. Kick, hold and Off commands follow `[Input] LeftHanded`. Opt-in
  remains `[PSVR2] AdaptiveTriggers=1`; default is OFF and the
  runtime/session/gameplay gates remain required.
- EVIDENCE: fresh x86 policy test passed; production DLL rebuilt `/O2` as x86
  PE32, SHA-256 `864844C7B3079172A4F44AAD987689557331FC6E87215EC96490279F776A8F95`.
  The current source also passed the Steam+xrsim integration build at SHA
  `10EC5F41…C8EBFFD`. The x64 bridge rebuilt as PE32+ at
  `deployment/psvr2-bridge-validation-20261009/SingularityPsvr2Bridge.exe` (SHA-256
  `177CFBE5E4D3C1A15D9D767D32B0223954DCE4978A5F8836BAE3EFFB3F89CDAD`); probe-only
  mode loaded the installed toolkit DLL and resolved its required exports without
  initializing hardware. The default workspace sandbox returned `WaitNamedPipeW`
  error 2, but an outside-sandbox dry x86-to-x64 smoke passed (`connects=1`,
  `sends=2`, `errors=0`); report: `deployment/psvr2-bridge-validation-20261009/dry-ipc-smoke-report.json`.
  This confirms the authenticated dry protocol, not an in-game client session.
- FOLLOW-UP: configured left-hand routing was added after the earlier `8648…` DLL.
  The x86 policy test (including a left-hand kick assertion) passes. The latest
  full x86 PE32 `/O2` candidate is SHA-256
  `8EF0DC084E1E5FEA9AE0A4C4BD73AB32D7DB0E5EA64C5B467CF8D84FFDFE48FA`, with
  HEAD-matched manifest. It passed Steam+xrsim replay of the 14:18 recording
  (1,420/1,420 batches; focused, two views, zero mono/stale submits/errors).
  Runner restored DLL/INI/VFX to the saved hashes. AdaptiveTriggers remained OFF,
  so this does not verify the trigger pipe or hardware effect.
- NEXT: verify an authenticated live game-to-helper session. Do not enable hardware mode or claim trigger feel until
  SteamVR/PSVR2 is available and the wall, kick, decay, disconnect-Off behavior are
  checked. Physical feel remains unverified.

## Workstream: ScriptFuncNameIdx crash guard and frame pacing — P0 remains open

- OWNER: main agent (`spikes/view_matrix/d3d9.cpp`).
- OBSERVED CRASH: the 2026-10-09 21:36 Steam test terminated with AV
  `0xc0000005` at installed candidate `d3d9.dll+0x42877`; WER records the bad
  read address `0x4800000c`. The candidate map and disassembly place the fault at
  the `UObject::Name` (`+0x2c`) read inside `ScriptFuncNameIdx`'s live FFrame
  sampling path, immediately after cached `Readable()` accepted a candidate
  pointer. The region may have been freed/reused before that dereference. This is
  the mapped fault path; the separate `dvr_xrsim32.dll` BEX report is secondary.
- CHANGE: candidate pointer, name-index, and `NameOf` reads in this probe now have
  narrow SEH guards, so a stale/unmapped engine pointer is rejected instead of
  crashing Singularity. `+0x10` had previously won 300/300 and 32/32 live samples;
  current code validates that measured slot 32 times at a 75% threshold and fails
  closed if it does not match. It no longer scans all 16 slots on every sample.
- BUILD / ROUTE: candidate `8EF0DC084E1E5FEA9AE0A4C4BD73AB32D7DB0E5EA64C5B467CF8D84FFDFE48FA`
  is x86 PE32 `/O2`, HEAD `6c457edb`. The measured slot logged 32/32. Steam+xrsim
  replay applied 1,420/1,420 commands; session FOCUSED, two projection views,
  zero mono/stale submits/errors, and process remained alive until runner cleanup.
  No later crash dump appeared. Default and one-variable `CutsceneAuto=0` runs
  both restored the exact Steam DLL/INI/VFX baseline.
- PERFORMANCE RESULT: the optimization does **not** resolve the P0 frame-pacing
  report. Both runs recorded initial `our work` residuals of ~34–37 ms, then
  ~11.1 ms after the initial menu/gameplay transition. `CutsceneAuto=0` had the
  same initial slow window without installing the script hook, ruling that hook
  out as the cause. Later xrsim menu windows were near 11.1 ms; the physical
  post-game-menu slowdown remains unreproduced. Do not call P0 fixed: the residual
  is not direct per-subsystem CPU timing and xrsim does not replace SteamVR/HMD.
- EVIDENCE: `../deployment/agent-test-bridge/20261009-215621/view_matrix.log`,
  `../deployment/agent-test-bridge/20261009-220013/view_matrix.log`, runner reports
  under `../deployment/agent-test-bridge/scriptfunc-seh-reports/`, and per-candidate
  replay JSON under `deployment/agent-test-bridge/`.
- NEXT: profile the actual SteamVR boot→gameplay→return-menu route with direct
  per-subsystem timings and compositor/GPU timing; retain the crash guard while
  investigating the remaining startup and physical menu hitches.

- MENU ENTRY RUNBOOK: before every visual/texture test, use the focused-window
  Enter sequence documented in `docs/SINGULARITY_GAME_ENTRY_RUNBOOK.md`. Confirm
  the final capture shows the live level, HUD and weapon; a title or main-menu
  frame is not accepted as gameplay evidence.

## DO NOT

- No VFX DLL to Steam before plugin desktop tests pass.
- Do not enable the experimental TMD arm-IK path (`ArmIKMode=1`) or broaden palette surgery without its own evidence gate. The user's P2 request authorizes only the existing exact-section independent-glove prototype (`IndependentHandsPrototype=1`, known shader layout + audited ranges); keep its bounded gates and do not mark it working without rendered-hand proof.
- No Coalesced/LOC binary patching (uepkg is read-only; repack = separate project).
- No guessing names/addresses/offsets (project rule, six runs paid for it).
- One variable per test run. Two agents never deploy to Binaries\d3d9.dll the
  same day without announcing (08:55 race burned a full test round once).
  A deployment also overwrites `Binaries\SingularityVR.ini`: after any deploy,
  re-check `ExtraCamReg` against the intended test value (shadow workstream above) -
  it was reportedly wiped once on 09.10 at 18:58:55.
