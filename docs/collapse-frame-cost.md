# Collapse frame cost (2026-10-18)

PID31100, DLL89966207: physical feedback describes a remaining dip to about72FPS
during the building collapse, otherwise acceptable. The matching gameplay log
contains77.4FPS/12.92ms (120-frame aggregate), peak3802 original draws, copy.01ms,
submit.27ms, gameplay hooks.02ms; no native controller enumeration in that span.
Later gameplay returns near90FPS. These observations do not separate game CPU,
stereo submission CPU, GPU execution or queue bubbles. In particular, the log's
`our work` is an unassigned residual, not exclusive mod CPU time.

The previous focused3600-frame profile (PID5860) estimates SetTexture at.7532ms
per frame, including nested texture bridge. Source inspection proves that one
Hook_SetTexture can call ProjectedShadowTexture three times with the same texture
and unchanged classifier state. Each invocation performs GetType even on a hit.

Hypothesis: memoizing that classification within ONE SetTexture invocation
reduces redundant CPU/COM work while preserving candidates, protection and stage
mask decisions. This does not cache across resource lifetimes or skip native
SetTexture/bridge callbacks. Default-OFF Render/TextureBindDeduplicate experiment.
Falsifier: baseline and optimized decisions/candidate increments/forwarded binds
differ, or a physical shadow/material/menu regression occurs. A runtime reduction
in classifier calls proves work reduction, not a locked90FPS result.

Diagnostic: a fixed ring of D3D9 timestamp/disjoint/frequency queries samples one
in8 frames from first native game draw to Present ENTRY (before our overlays/XR
copy). Poll only with GetData flags0, never flush/wait; discard disjoint, invalid,
mixed menu/cinema and unsupported results. A GPU timeline interval can include
submission starvation/bubbles and is NOT exclusive GPU busy time. Compare it
with matching first-draw-to-Present CPU wall, full-frame wall, draw count and the
existing bounded hotpath sampler. No screenshots or per-draw log output.

Query contracts: https://learn.microsoft.com/en-us/windows/win32/direct3d9/queries

PID31100 exited; installed DLL789749E5/INI8B0B0B4C/injector5B948DDB independently
rehashed after guard restoration. No persistent promotion authorized. Camera
comfort flag was present, but no native-hook apply/installation lines were found;
physical improvement alone must not be attributed to that hook.

Implementation, L1/L2 and runtime results follow below; no performance gain yet
claimed. Preserve the last candidate's projector/muzzle/camera/input behavior.

## Offline result and next physical check

L1:672 flag/stage combinations preserve stage mask, protected-source decision,
candidate increments and classification miss counts; triple path3->1 classifier
invocations. A subsequent bind always reclassifies its own texture. Mocked GPU
tests cover delayed data, full ring/drop, disjoint, zero frequency, backwards
timestamps, mixed scene rejection, partial allocation failure, driver error and
Reset during a recording. Existing sampler, projector math/scopes, camera ABI
and controller cache tests pass (seven C++ executables).

Standalone hidden32x32 D3D9Ex fixture: ten issued, ten valid, zero skipped,
invalid or failed; render state unchanged. This is real driver support evidence,
NOT Steam/PSVR2 or a performance result. Hidden-window Present did not submit
the initial fixture's workload: that first attempt produced no completed samples.
The fixture now requests queue submission with its own event (one FLUSH poll,
no busy wait) and yields1ms per rendered frame. Production probe always uses
flags0 and relies on the game's existing presentation/XR-copy queue submission.

Candidate preparation adds only Render/TextureBindDeduplicate=1 and
Diagnostic/FrameGpuProbe=1 versus PID31100's INI. Rendering quality, resolution,
native game simulation, localization, hands, knife and reload are unchanged.
GPU_SCENE_WORST includes correlated costs/draw count for the same sampled frame;
GPU_SCENE_WINDOW is cumulative and must not be mistaken for an instantaneous FPS.

After fresh start: verified install/backup/600s launch lease; arm existing
request-gameplay-hotpath.ps1 -AfterCinemaExit (bounded45s/3600frames after
cinematic exit). If the save starts directly in gameplay, use the generic
request instead. Stay in PSVR2: closest crash save -> walk through the collapse
-> another15s gameplay -> normal exit. Inspect input-cache, texture decisions,
GPU scene timeline, CPU sampler and invalidation counters together. Restore and
independently hash the safe install; remove an unconsumed owned request only
after confirmed exit. No screenshots, no mid-test chat, no quality reduction.

L2 build from6201c193a4f7de4de6d1051970ae1115f2ae56a4, x86 PE32/O2;
DLL `3ED38B9603D7FAE6DCA11141DF939AA5315EDDB4394626EB1C4DD1D389AA7020`.
INI `0EFC595BBBB70B0DE0066081550BF089A13B389C871F204DC6CE90E79669789F`.
Offline package `build/projector/candidate-collapse-cost`; approved-profile
Install -WhatIf passes without Steam writes. Candidate is NOT L3/L4 verified.
The prior PID31100 test did not arm a3600-frame request: only aggregate counters
exist for that run. Next test MUST arm the existing request before launch.

## PID22160 physical result and measured boundary

User: remaining FPS drops; hands/image good. Safe DLL789749E5,INI8B0B0B4C,
injector5B948DDB,UI51CDAB5C independently rehashed after guard restoration.
Private evidence-22160 logSHA2568A4A0F47B059F8827BAD191BE983178BB2CC65B143A371C2E1F40445D9A45895.
The owned request was consumed. Last sampler report3120frames is PARTIAL;
there is no complete3600frame result. Frame scope also later includes a return
to cinematic/menu, so do not merge that with the measured collapse interval.

The collapse aggregate itself is72.9FPS/13.710ms, peak3459original draws,
present-thread CPU10.807ms/frame; process-wide user23.177+kernel3.906ms is
summed across threads, NOT additional wall time. Fence wait2.84ms, copy.01ms,
XR submit.27ms, gameplay hooks.02ms. This does not fully name a native function.
One native enum64.664ms occurs earlier, separately from the collapse interval.

273GPU samples completed with no dropped ring/invalid/query errors. GPU timeline
has a19.573ms maximum during the early sampled interval. That sample belongs to
the CPU-wall-under-budget bucket: GPU commands execute asynchronously, so the
bucket alone cannot exonerate GPU workload or attribute the following CPU stall.
Average GPU timeline over the whole active period is likewise not a worst-frame
budget. Preserve this limitation in all summaries.

By3120frames texture memoization removed6952375 redundant classifications.
This proves that optimization executed, not that the remaining dip is fixed.

### Falsified offline wait experiments

The accepted zero-copy path repeatedly polls GetData with FLUSH while waiting
for GPU completion. This consumes render-thread CPU, but does not prove changing
the wait can shorten GPU execution. Test two alternatives before any Steam patch:
yield after100us (SwitchToThread), or FLUSH only every64polls. Same completion,
100ms timeout and error behavior retained; synthetic tests pass.

Standalone D3D9Ex,1280x720,2048clears/frame,ABCCBA,80frames/policy:
native spin wall2.2699ms/8,289,025cycles per frame; yield2.2988ms/8,426,533cycles;
coalescedFLUSH2.2843ms/8,324,598cycles. Zero failures for all; thread CPU times
are coarse and report2.3438ms for all three. Neither improves wait latency or
CPU cycles. Thus neither is integrated. Source is ONLY under tests/support and
tests/benchmark-gpu-fence-wait.cpp. Do not present fewer polls as an FPS fix.
Reference: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-switchtothread

### Next productive measurement

After two unsuccessful mechanisms, use actual sampled CPU call stacks to find
the producer. Existing Windows tools WPR/xperf/WPAExporter are available; current
token is non-admin. No recorder has started. Built-in CPU.Verbose would allocate
1592x1MiB plus20x1MiB buffers on this host, so a compact custom profile is now
provided: tools/profiling/SingularityCpu.wprp,128x1MiB circular memory buffers,
CPU/IP stacks,process/image identity and scheduling events, no user providers.
This bounds capture buffers, not an absolute final ETL/rundown file size.
WPR -profiledetails validates exact128 buffers and only SampledProfile stacks.
Reference: https://learn.microsoft.com/en-us/windows-hardware/test/wpt/logging-mode

Next fresh start: use exact previously tested candidate3ED38B96, arm the existing
cinema-exit request and restore guard. Start hidden recorder with Windows UAC
before headset entry:
tools/record-trigger-cpu.ps1 -CompactProfile -CaptureSeconds30 -AfterCinemaExit
-LaunchWindowSeconds180 -CinemaExitWindowSeconds300.
It waits for this process's new cinema-exit marker; no chat/keyboard during play.
Trace stays private/system-wide; analyze only Singularity PID and its render
thread, module RVAs and unique sampled stack-frame hits. Check lost events and
trace time coverage before causal claims. No new shader, quality or wait-policy
change is authorized by these offline experiments.
