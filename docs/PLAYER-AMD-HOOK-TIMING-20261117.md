# Optional gameplay helper timing (2026-11-17)

Branch codex/singularity-player-perf-20261117, parent 6f4acd6f (optional Pico
calibration, default OFF), controls lineage 3559d949. The player's exact DLL
hash/INI has not been verified. Do not call this their accepted build.

[Diagnostic] GameplayHookTimingExtra=0 default. With 1, the two helpers not
covered by existing per-helper timers are measured: ReportEyeHeight and
NativeRifleFx::Install. Output at the existing frame-budget cadence reports
total ms, WORST call and calls. These scopes are nested in gameplay hooks,
not additional costs to sum. No new GPU query, wait, hook or per-draw timer.
Do not combine a probe run with quality, driver, cache or runtime changes.
The optional [Render] TimeOurWork=1 remains independent and measures draw
processing, not these helpers. It adds per-draw QPC overhead of its own.

Read complete player log at Downloads/view_matrix.log: line 544 is the real
one-time SolvePropertyLayout 199.4/199.4. Line 3789 shows gameplay hooks
worst 67.11 ms, while line 3790's four scope peaks are <=0.7 ms; native rifle
hook arming is logged at 3741. This is a coverage gap and correlation, not
proof of the hook-install or scheduler cost. Source instrumentation adds no
new behavior to those helpers. SolvePropertyLayout itself is unchanged.

Offline: spikes/view_matrix/build.ps1 -Performance, MSVC x86 /O2. Build SHA
and provenance staged privately under build/player-perf/. Runtime, AMD and
physical headset NOT RUN. Source status IMPLEMENTED_NOT_VERIFIED. No local
Steam install, release or installer changes. Before a remote trial preserve
working DLL/INI, compare exact hashes, then toggle this single diagnostic;
restore 0 and the original DLL afterward. This is tooling, not a mod fix.

### Eye-height diagnostic polling, November 27

With EyeHeightLog enabled the throttle previously counted from the last emitted line. Once the pawn had a stable base height, the helper sampled it every frame after the two-second interval expired. A missing pawn also retried every frame. A deterministic x86 harness exercised the production function over 375 frames at 16 ms intervals. The original sampled 251 times for stable height, 375 times without a pawn and 127 times after one height change. Timing from the last sample bounds each of those cases to three reads over six seconds. The respective emitted height lines stayed at one, zero and two. The disabled case made zero reads in both versions. The AMD player's supplied log had no EyeHeightLog enable marker. This finding applies to optional diagnostics; the reported AMD frame spike remains open.

### Research Facility replay, November 27

Steam app 42670, x86 candidate `cdf741f5`, dvr-xrsim, 1920×1080, 90 Hz. Loading the original Research Facility slot reached stereo gameplay at frame 8383. The inventory reader identified four owned weapons and an empty Seeker slot. The game profile checksum was identical before and after the replay.

In the window containing native direction and emitter activation, the gameplay helper peak was 63.74 ms. One call to NativeRifleFx::Install took 63.728 ms, out of 63.777 ms total across 120 calls. Eye-height's maximum across all 145 windows was 0.001 ms. At startup the separate 112.87 ms gameplay peak included a 105.5 ms property-layout solve. The two stalls occur at different points in the run. This reproduces a similar gameplay stall on the local NVIDIA adapter. The AMD player's original recording lacks per-helper measurements.

The optional GameplayHookTimingExtra flag reports successful tracer and emitter setup stages. Timing the existing pair of movement symbol lookups on a read-only mapping of the same executable took 5.97–9.40 ms over four trials. During the measured emitter activation in gameplay, the movement bindings were already cached.

On a second Research Facility load, emitter contract validation took 0.252 ms. Direction activation took 33.546 ms and emitter activation took another 34.695 ms in the same frame. Queuing the two audited targets together reduced activation to 33.644 ms in a third load. The maximum gameplay-helper time in the corresponding windows dropped from 68.52 to 33.94 ms. Both runs produced eight bounded first-person muzzle receipts and identified the native emitter after firing. The tracer still activates separately. The game can still hitch at setup and the AMD player's original spike remains unattributed on their hardware.

The release build and native muzzle regression suite passed. An x86 fault-injection harness exercised the production setup block with both targets, with the direction feature off, with a failed fingerprint, after each of six setup failures and with either cleanup failing. Failed cleanup retains the affected callback pointer and ends retries. Successful cleanup allowed retrying.
