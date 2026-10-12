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
7. Gloves-only model (NEW, 09.10): log says `glove isolation=0` = OFF, and
   `ApplyGloveIsolation` returns at `if (mode <= 0)` before its own census line —
   which is also why `GloveCensus` never logs. Root cause: source INI carries
   `GloveIsolate=0` while its own comment says *"use the live wrist path"* (=2), and
   the glove integration doc says installed INI had `GloveIsolate=2`; a later deploy
   copied source over it. Deployed INI restored to `GloveIsolate=2`.
   USER: with `GloveIsolate=2` present, judge GLOVES ONLY (not shadows) — arm flesh
   should collapse onto the live wrist and `GloveCensus ... mode=2` should appear.
   Then holster (right grip rising edge) and check whether the stored weapon stays at
   the hip when the hand moves away — that item is still unproven.
   Detail: `docs\HANDOFF_GLOVEISOLATE_20261009.md`. Do NOT stack a shadow variable.

## Workstream: shot-fired PSVR2 trigger resistance

- OWNER: main agent for production wiring; trigger policy originated on
  `feature/trigger-shot` @ `ca105a8`.
- STATE: `trigger_shot.h` and `docs/TRIGGER_SHOT.md` copied into this checkout. The
  policy header is staged only: current production `d3d9.cpp` does not include or
  call `psvr2trigger`, `psvr2client::Client` is not started, and there is no x64
  named-pipe bridge executable in this checkout. So no in-game trigger command or
  shot kick is currently active.
- NEXT: integrate against the actual gameplay/trigger lifecycle and launch-config
  contract, then build the x64 bridge before any headset feel test. The source
  branch's `PASS: shot-fired trigger resistance policy` is its own reported result;
  it has not been rerun in this checkout.

## DO NOT

- No VFX DLL to Steam before plugin desktop tests pass.
- No live-IK/palette writes (other agent's gate).
- No Coalesced/LOC binary patching (uepkg is read-only; repack = separate project).
- No guessing names/addresses/offsets (project rule, six runs paid for it).
- One variable per test run. Two agents never deploy to Binaries\d3d9.dll the
  same day without announcing (08:55 race burned a full test round once).
  A deployment also overwrites `Binaries\SingularityVR.ini`: after any deploy,
  re-check `ExtraCamReg` against the intended test value (shadow workstream above) -
  it was reportedly wiped once on 09.10 at 18:58:55.
