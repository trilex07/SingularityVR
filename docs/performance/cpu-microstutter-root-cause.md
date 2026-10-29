# SingularityVR CPU microstutter: evidence and one capture candidate

## Verified baseline and boundary

This branch starts from `origin/main` at `a1b506b9` in an isolated worktree.
The accepted event-invalidated DirectInput enumeration cache remains configured
ON by default (`[Input] InputEnumerationCache=1`). The historical 75–76 ms
device-discovery stall is not a new hypothesis. The existing `HotPathSampler`
and `FrameSpikeReport` are default OFF. The latest read-only local
`view_matrix.log` (2026-10-29 23:19, SHA256
`EB5D1BEBD09235EB696C39000F5DFCCBB8DE49CCF17E91B720E3E92ACEB647BE`)
has no `FRAME SPIKE` records and does not show a controlled combat/pickup run.
It cannot establish a current firing or pickup bottleneck.

The game already has confirmed native script entry points `FireAmmunition`
and `TakeDamage`; a change in `Pawn.Weapon` object identity is the accepted
weapon-switch event. The raw Sense `Use`/Grip rising edge is observable but
does **not** prove an ordinary world-item pickup. WeaponLocker, magazine
pickup and item pickup must not be lumped together.

## Candidate

`[Diagnostic] CpuMicrostutterProbe=1` starts a 9000-gameplay-frame, default-OFF
trace. It records end-to-end Present-to-Present wall time, this frame's Present
hook wall time, the forwarded D3D9 Present call, CPU consumed by the render
thread inside this hook, native fire/damage call wall durations, and the four
event markers above. A fixed 512-frame ring retains several seconds of
history. A below-normal-priority worker drains it every 100 ms to
`%LOCALAPPDATA%\SingularityVR\cpu_microstutter_<PID>.csv`. No filesystem I/O,
heap allocation, stack walking or GPU readback runs in the render/input/script
hooks. The trace pauses outside focused stereo gameplay; menu/loading frames
do not exhaust the 9000-frame limit.

The CSV is raw. The reusable `tools/perf/analyze_microstutter.py` reports
p50/p95/p99/max frame wall time, counts over 11.11/16.67/22.22/33.33 ms and
seven-frame windows around each event. Its `use_grip_attempt` label is
intentional: a successful pickup producer has not been proven. Event proximity
is not causal attribution. `frame_wall_us - hook_wall_us` is **not CPU time**;
it includes game work, graphics-driver work, waits and scheduling. `GetThreadTimes`
only covers the Present-calling thread while inside this hook. The native
call durations are wall times, not thread CPU times. ETW CPU sampling and file
I/O/page-fault evidence remain necessary to name a root cause.

## Offline validation

- `spikes/view_matrix/build.ps1`: x86 PE32 `/O2` PASS; no Steam install.
  Build from implementation commit `47c7b6af`; DLL SHA256
  `F4744971DE7E751FD36F9AEAE7816CE6D55D255BE6469190C917B451FCA41A8C`.
- `spikes/view_matrix/tests/test-microstutter-probe.ps1`: actual fixed ring,
  native duration, OFF gate and asynchronous CSV flush PASS. Synthetic frame
  instrument cost was approximately 1.04 µs/frame on this workstation.
- `python -m unittest discover -s tools/perf/tests -p test_analyze_microstutter.py`:
  two analyzer tests PASS.

The benchmark measures only the probe on this machine. It is not a game FPS
claim. The follow-up historical audit is in
`docs/performance/historical-cpu-stall-audit-20261030.md`. It identifies one
deterministic unnecessary cost: `MenuDrawTrace` was default ON and made
synchronous D3D9 state queries during transition samples. This branch makes
it opt-in through `[Diagnostic] MenuDrawTrace=1` and avoids its per-frame
state work while disabled. No physical FPS improvement is claimed. The
available evidence still does not justify changing native firing, pickup,
haptics or effect behavior; the accepted Steam baseline is untouched.

## One controlled MAIN capture

MAIN alone backs up and hashes installed DLL/INI, reviews this branch, builds
the exact commit and deploys the candidate with only
`[Diagnostic] CpuMicrostutterProbe=1`. Do not enable other heavy diagnostics
in the same run. In one focused PSVR2 scene: idle 10 s, 5 spaced single shots,
one automatic-fire burst, hit a living enemy, pick up one ordinary ammo/health
item, switch weapons and interact once. Close normally; archive the complete
CSV, `view_matrix.log`, effective INI and DLL hash before relaunch. If the
ordinary pickup has no unique marker, annotate its wall-clock time or pair the
CSV with ETW rather than claiming the grip edge was a pickup.

Analyze with:

`python tools/perf/analyze_microstutter.py <capture.csv> --json <report.json>`

Compare event windows with non-event gameplay frames and, where available,
WPR/ETW CPU sampling stacks and hard-fault/file-I/O data. Keep native D3D9
Present and XR waits separate from CPU execution. A justified fix requires
the stack or exact hook whose time increases at the spike; then add one
reversible change and rerun a comparable scene. Only MAIN can establish
Steam/OpenXR/physical PSVR2 results.

## Falsification and rollback

The hypothesis that a mod CPU hook causes firing/pickup stutter is weakened if
spikes occur without those events, or if ETW places the extra time in UE3,
driver waits, hard faults or the compositor. A missing/empty CSV or
`lost_frames>0` or a missing footer (`lost_frames=-1`) limits event-rate
conclusions; do not fill gaps by
interpolation. Turn the INI flag OFF and restore the verified accepted DLL/INI
if any visual/input regression appears.
