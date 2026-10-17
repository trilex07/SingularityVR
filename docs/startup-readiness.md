# Startup hook readiness (SIDE, 2026-10-17)

User reports a visible but temporarily uncontrollable world after load; requests
faster hook initialization and a loading view until ready. Cause of native move
blocking is NOT yet established. No change to UE3 IgnoreMoveInput/cinematic
state, input cache, saves or controller bindings is authorized by this experiment.

Evidence: current source FindPlayerPawn scans at most512 objects/1ms, one slice
per16ms. Session14:46 acquired its pawn at index114610 (at least224 slices),
after scanning112565 objects without a pawn during splash. Script-name retries
wait240 frames and trace retries120 frames, so delay depends on frame rate.
Cutscene auto-init currently marks itself hooked even if installation failed.
No timestamps yet prove what portion of user-visible delay each contributes.

Hypothesis: resolve Controller.Pawn and reciprocal Pawn.Controller by existing
UE3 reflected property names, validate live class/ownership, and use this direct
link before the incremental fallback. Do NOT raise the scan budget. Rearm
gameplay-offset retry when a new pawn is acquired; use bounded wall-clock hook
retries only when explicit StartupPrewarm opt-in is enabled. Add bounded elapsed
readiness/timing logs so MAIN can compare the same save without guessing.

Separate default-OFF StartupLoadingGate is PRESENTATION ONLY: cover the first
gameplay frame until enabled core hook/pawn/XR-input checks are stable100ms, or
max5000ms timeout (clamped). Menus, pauses, Bink and cinematics bypass it. No
extra input blocking/pause/time dilation/engine loading modification: simulation
continues underneath. Timeout must expose the world and log missing checks,
never hide it indefinitely or conceal a broken hook. Native controls may still
be intentionally blocked by game scripting: these checks are NOT proof that
the engine accepts movement. Do not claim the native loading screen was extended.

Falsifiers: reflected links absent/mismatched (fallback retained), no reduced
acquisition time, readiness passes but user cannot move, curtain masks active
cutscene/menu, timeout hides world permanently, state/Reset or stereo regressions.
MAIN measures actual wall time from first gameplay to core-ready and verifies
movement using the same save. SIDE performs L1/L2 only, ZERO Steam.

Frozen source log: `build/startup-readiness/session-1446.log`, SHA256
`163AD7DD3012E8245A843DE3252E7B25431B0014C975655E63530142B34D72D4`.
No before/after wall-time improvement has yet been measured in the game.

## Implemented scope and MAIN experiment

`[Input] StartupPrewarm=0`: fast reciprocal reflected pawn link with original
fallback. No FindController/global object scan added to that fast path. Class
and live PersistentLevel checks, aligned guarded loads, reciprocal owner match;
no hardcoded property offsets. The cached pawn path also rechecks liveness in
opt-in mode. Reflected offsets are cached by class; missing names retain fallback.
New-pawn acquisition rearms gameplay property retry. Missing script names/trace
hooks retry immediately once camera exists, then250ms/1000ms wall-clock backoff,
with the existing attempt caps. Failed interpreter-hook installation retries
instead of being falsely latched as installed. Flag0 retains previous scheduling.

`[Diagnostic] StartupReadinessProbe=0`: bounded64 readiness transitions with time,
identity, mask/missing checks and timeout result. Mask bits: pawn1, property
layout/Weapon2, XR move action/both hand positions4, enabled script hook8,
enabled aim trace16, centred head/camera/eyes32. Optional/absent assets are NOT
declared loaded by this mask; engine move acceptance is not inferred from it.

`[Render] StartupLoadingGate=0`, `StartupLoadingMaxMs=5000` (500..10000),
`StartupLoadingMinMs=3000` (0..max): same mask feeds a minimum-duration,
100ms-stable curtain only on eligible focused gameplay. It stays visible until
both the minimum display time and stable hook-readiness are met; the maximum is
still a fail-open timeout. Intro/Bink,
cinema, menu/settings, pause bypass immediately. No new ClearPad, movement veto,
sleep, simulation pause, state/timer or file write. The world continues behind
it; MAIN must NOT leave a timeout looping or claim this extends native UE3 load.
This is a bounded neutral two-eye loading view, not reuse of proprietary loading
art. On timeout it exposes the world and logs missing hooks. A failed draw/state
restore disables the curtain after restore failure; no repeated corruption.

The curtain is the LAST overlay before BOTH GPU/CPU backbuffer capture paths,
after the same frame's XR input poll. Draw uses original RT/viewport/Clear APIs,
no shaders/texture allocations, no private depth, no retained COM resources.
RT/depth/viewport/scissor enable AND rect are restored. Own GPU testing proved
SetRenderTarget resets scissor rectangle too; the initial fixture caught this,
then explicit scissor-rect restoration passed. No mesh/weapon/lighting changes.

MAIN first integrates/builds this SOURCE delta on its current accepted profile.
Probe-only baseline: same save with StartupReadinessProbe=1, other new flags0.
Compare StartupPrewarm=1 in a second session (same save/location). Inspect
reciprocal acquisition, elapsed-to-ready and missing mask; verify movement,
first aim/fire, hands, pause/Options and return to menu. Then opt-in the loading
curtain and verify both-eye matching, correct release, no cutscene/menu cover,
timeout/manual-menu escape. User remains in PSVR2 until normal exit. Roll back
on failed core behavior; preserve all unrelated accepted settings/assets.

Offline L1: `tools/tests/test-startup-readiness.ps1`:6 executables (policy,
own D3D9Ex GPU, scene/cinema/input/hand regression),5 Python source guards.
GPU2 Reset rounds:546/546 matching pixels, RT/depth/viewport/scissor restored
on success, failed viewport and failed clear. Synthetic glyph rectangles only,
not real game loading pixels, font legibility or physical acceptance. Timer
tests cover each missing gate, stable debounce, late readiness, timeout, clock
fault, pawn replacement, pause/menu/cine bypass. Log build/startup-readiness/tests.log.
Native ownership link correctness and speedup remain runtime UNKNOWN.

## Clean L2 candidate / delivery

Source `e243f97fc8354c37391d11609ec5b9bc972d806d`, dedicated branch
`fix/startup-readiness` from origin/main92396ca (accepted Centurion source kept).
L2_PASS clean source, x86 PE32 014C/010B, `/O2`, performance instrumentation
OFF; existing unrelated analyzer/compiler warnings remain. DLL3211264 bytes,
SHA256 `CE297F0AF8BADFAE941FB500BE8A07D9D139DCCF15E427FCABD2A2CC3867F003`.
Build receipt SHA256
`26751FE75790064595BAA954749102CBA4AA0CBB6E27A26740E3E747F3E2F178`.
Build log: `build/startup-readiness/build.log`.
Private `build/startup-readiness/candidate-e243f97/`: DLL/build receipt/manifest
only, NO INI/assets, no accepted/installed/restore identity inferred or changed.
SOURCE integration/rebuild by MAIN on current profile, not blind old DLL deploy.
All opt-ins default OFF. Runtime baseline/fast path/curtain remain UNKNOWN;
L1/L2 cannot establish actual load-time reduction or movement availability.

## MAIN Steam / PSVR2 acceptance and stable promotion (2026-10-17)

The user accepted the combined startup-curtain and Centurion tracked-cylinder
candidate in the physical PSVR2 test. The scoped acceptance is the tested route;
it does NOT prove that all UE3 content finishes streaming behind the curtain or
that pawn/hook acquisition became faster. Those remain separate measurements.

Candidate `c0393dd` was promoted as a fast-forward to `main` at
`95a04388e2340b414518192b79845003ac520f8c`; intervening commits were documentation
and restore-lease bookkeeping, with no change under `spikes/view_matrix/`.
Persistent Steam install was made through `tools/deploy-singularityvr.ps1` after
the test restore had completed and the game was confirmed closed. Installed DLL
SHA256 `789749E5D4DB8FC6C603DF599A38A4FB1EC00DE374E4E2FDD469B09B9C5F361B`,
candidate INI SHA256 `8B0B0B4CF2DB78514A44B72CF3E79CC1C48C74E5632CDA34A468CA61702CCFAF`,
unchanged dinput8 SHA256
`5B948DDBDE07F16AE779C274CEBA133F7F24A218ABD9FFF2D4133FCADF729D59`.
Backup/receipt: `deployment/install-backups/20261017-194405` and
`deployment/install-reports/20261017-194406.json`. Startup curtain uses the
3000ms minimum with existing 5000ms fail-open maximum; it is not an engine asset
streaming barrier. All four reviewed Centurion follow-up switches are enabled.
UI package and all other protected assets were retained.
