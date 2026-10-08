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
- White ring right-eye-only (12:54:08 shot): per-eye sprite artifact, NEW instance
  of the muzzle-on-seam class. SpriteQuadPerEye=1 set in user ini; verdict pending.
- Menu-draw census (MDG groups) flows since 315ce56. 470 groups/session typical.
- L3+R3 = screenshot BMP next to log (+also recentres, pre-existing). Debounced 3s.

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

## DO NOT

- No VFX DLL to Steam before plugin desktop tests pass.
- No live-IK/palette writes (other agent's gate).
- No Coalesced/LOC binary patching (uepkg is read-only; repack = separate project).
- No guessing names/addresses/offsets (project rule, six runs paid for it).
- One variable per test run. Two agents never deploy to Binaries\d3d9.dll the
  same day without announcing (08:55 race burned a full test round once).
