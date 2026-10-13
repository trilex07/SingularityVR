# Remaining frame dips after texture cache repair

2026-10-13, MAIN branch fix/cutscene-physical-observation. No new Steam session
until the user explicitly writes `start`. Prepare offline first; accepted
installation remains live, not an unattended candidate.

## Observed, not inferred

PID43712 used VR64353686 + injector1BC0F7DD. Resource-cache counters ended at
14,145,976 binds, 9,909,759 native hits, 668,586 legacy hits, 950 hashes,
914 stored decisions, zero failures/evictions, table36/512. Late snapshots
retain exactly950 hashes. User confirms L2/R2 no longer reduce FPS. Smaller
remaining dips and stretched hands persist; texture appearance was not
explicitly accepted by the user. Restored DLLs D8928180/AAA07005, current
INI B3FEED43 and UI5B2EA469 verified, all107 DDS files unchanged.

The old pre-cache trace is not a profile of this new build. Current native
120-frame windows include 68.9FPS/14.51ms with peak3803 split draws, versus
later90FPS windows with lower draw peaks. These are peaks, not exact average
draw counts; correlation does not identify a contributing draw or GPU time.
Copy CPU ~0.02ms, XR submit ~0.34ms, input sync ~0.17ms average in that slow
window; a 2.27ms fence wait is not complete GPU-frame time. `our work` is the
unaccounted frame residual INCLUDING engine/driver work, not mod CPU alone.
Whole-process CPU >50% from user observation is not a function-level cause.

## Falsifiers before another performance patch

About3.57million binds were neither cached hits nor hash calls. Source has a
real missing memoization case: a successful noncandidate GetLevelDesc result
is rescanned against the static target shape table on every bind. Shapes are
immutable for a resource lifetime, but repeated work is not proof of a large
cost. First measure an optimized synthetic native-D3D9 reproduction with the
exact target table and compare a stamped resource, without game/mod proxies,
Steam, DDS assets or driver settings. Report microseconds/call, never claim
game FPS from this synthetic benchmark. Do not patch if the cost is too small
to explain observed milliseconds of residual.

Next runtime candidate should separate injector classification CPU from the
forwarded SetTexture CPU, plus the already-existing bounded VR hook sampler
after actual cinema exit. No drawing/state/texture/controller changes. Timing
must be sparse, fixed-storage, bounded, and OFF in normal builds; no QPC or
filesystem writes per bind. A negligible injector classification time in a
slow window falsifies remaining injector-CPU attribution and redirects the
investigation to measured engine/render hook/driver work. Inclusive scopes
must not be summed or presented as GPU timings.

Hands remain a separate open correctness issue. Current user configuration
ShowControllerArms=1 is retained. No silently selecting glove-only, shadow,
quality, resolution or input changes to improve a performance result.

## Offline scan result and next candidate

Native system D3D9, x86/O2, exact80-target table,5x2,000,000 calls: current
noncandidate check0.0540–0.0614us/call versus stamped0.0149–0.0183us/call.
This excludes the global positive-table lock/lookup and real SetTexture. It
does not measure game/GPU time. At the observed uncached call count it cannot
plausibly explain the multi-ms frame residual by itself. No shape caching
patch is applied. Keep positive lookup and asset policy unchanged.

Injector diagnostic build switch `-DiagnosticTiming` is OFF by default. It
samples roughly1/1024 eligible binds with per-thread xorshift selection to
avoid fixed-period draw-list aliasing; cap20,000 samples and300s from init.
Fixed atomic storage, no per-bind allocation/file write. Classification and
forwarded SetTexture wall durations separated by route; original pointer,
stage, HRESULT and exactly one forwarding call retained. GetType early
bypass and code outside this hook are not measured. QPC/scheduler delays can
inflate CPU wall durations; none are GPU times. Worker snapshots are live
aggregate estimates, not synchronized frame traces. Use deltas between warm
snapshots, not cumulative startup averages. Existing45s/3600-frame VR sampler
after real cinema exit supplies complementary inclusive scopes. Do not sum
nested scopes or infer native engine CPU from Present-hook residual alone.

L1 checks: sample selection/rate, hard sample cap, stopped/disabled scopes,
separate classification/forward accounting, preserved HRESULT and exactly
one forwarding call. Actual HookSetTexture tested on native synthetic HAL
resources: disabled/null/noncandidate/stamped/legacy-negative/positive
replacement paths retain bound pointer and stage; noncandidate remains
unstamped. Existing655-resource pixel/reset/lifetime checks still pass.
No claim of real-game post-cache attribution before the next Steam session.

## Prepared test (execute ONLY after user writes start)

Stage directory: `deployment/texture-cache/post-cache-diagnostic`. Reuse
VR64353686 from sourcef57c3d5 exactly; new injector is diagnostic-only with
the already-executed negative cache. Verify both PE32/O2 manifests and hashes
and recheck game closed + originalsD892/AAA + frozen INI/UI/107DDS.
Do not install or arm requests/guards while waiting for start.

After start MAIN uses deploy-singularityvr.ps1 `-RuntimeDllPairOnly` with
this explicit stage directory. Timestamped backup must contain exactly two
original DLLs and no directories. Verify installed candidates against build
hashes, launch the pair guardian with `-CleanupCinemaExitRequest`,600s launch
lease and post-exit restore, wait for READY. Create the owned request through
request-gameplay-hotpath.ps1 `-AfterCinemaExit` (abort on a pre-existing
request), then arm the read-only observer and launch ONLY Steam42670. No UAC
profiler or tab switching required. If staging fails before request creation,
rollback with verified pair tooling. If launch fails the guardian removes
only this exact unconsumed request after safe restore.

Physical script, agreed BEFORE headset entry: Continue from catastrophe;
after stereo returns stay ~10s without triggers, L2~5s, R2~5s, then ~30s
normal movement/looking around. Close the game normally. No return to chat
mid-session, no second New Game in this attribution round. Early stop if
uncomfortable. Report where dips occurred, hands appearance, and whether
texture appearance matches the preceding cache test. No test claims for the
hands fix: there is none in this candidate. Automatic native request waits
for true cinema exit and captures max45s/3600frames; a shorter session yields
partial progress, not a fabricated full profile. Copy injector log after
exit (held open while running); verify pair restore and all frozen hashes.

Offline release/diagnostic injector builds and static format analysis PASS;
default-OFF compiled test preserves10000 forwards withzero samples. Guard
syntax checked in PowerShell7 and native Windows PowerShell5.1. Latest fresh
read-only installed/backup/DDS/config/UI verification PASS. No next candidate
is installed, no profile request written, no game launched in this turn.

## Post-cache Steam result PID33140 (2026-10-13)

VR64353686 / injectorD6EFDDAE, source8438447. Steam/OpenXR/timing execution
confirmed. Profile completes3600frames/40,625ms; bins3408/126/66/0,229menu
frames and0cinema frames. NOT a pure-gameplay aggregate. User confirms dips
are during gameplay; no further texture/hand/second-entry result supplied.
Subsequent video/cinema entry is recorded, not full second-entry acceptance.

Texture counters100s:8,017,646binds/5,739,983native hits/366,240legacy hits/
850hashes/803stamps,0failures/evictions,table47/512. Routine classification
sampled mean~0.104us native/~0.238us noncandidate; no recurring old hash churn.
Only3sampled hashed-native calls: insufficient to bound cold asset-loading
cost. Do not implement the rejected shape-cache micro-optimization.

Three native scans average43.712ms: one after cinema exit, another in menu,
third after resuming. Cache intentionally invalidates at these boundaries.
Slow77FPS/12.99ms gameplay window(line5927) has LT/RT0/0 and peak3330split
draws. Discovery AND callback replay totals are unchanged between480/720frame
reports: cannot explain this window. WMI misses remain2 throughout(hits134).
Leave accepted input cache/recovery policy intact.

Existing draw scope is INCLUSIVE,~1.78ms/frame by720frames. GPU-fence wait
1.76ms in slow window is not full GPU frame time. Falsifiable next probe:
separate native Draw* CPU submission from inclusive draw wrapper time, using
existing ShadowTrace forwarding boundaries (no new hook). If native forward
cost dominates, investigate engine/driver workload; if small, investigate
mod wrapper's actual hot scopes. Independent estimates are not exact additive
CPU/GPU accounting. No performance/rendering fix claimed from this evidence.

Native forwarding family uses~1/512randomized timing samples during the bounded
sampler only. Preserve parameters,exactly one original call,HRESULT,shadow
trace and state; no allocations,per-draw logs or COM-state queries. Thread-
local RAII nesting excludes standalone overlay/VFX calls outside profiled draw
hooks and calls from other threads. Stop never resets scopes in flight.
Default OFF unless owned gameplay request runs; no image/hand/input changes.
Randomized selection avoids always sampling the same eye of adjacent stereo
draw pairs. Existing unrelated timing families keep their previous policies.

Guard31268 restored originals from20261013-203555; MAIN independently rehashed
D892/AAA,INI B3FE/UI5B/107DDS/forwarder unchanged,request consumed. Game closed.
Prepare next candidate offline; no further Steam run before fresh user start.
