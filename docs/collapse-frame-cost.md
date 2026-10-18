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
