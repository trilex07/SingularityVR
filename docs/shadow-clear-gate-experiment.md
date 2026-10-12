# Native shadow-depth clear classification experiment

Status: RUNTIME_CLASSIFICATION_PASS / PARTIAL_PHYSICAL_RESULT, default OFF.
No shader correction; square-shadow defect remains unresolved.

## Same-session runtime measurement, 2026-10-12 PID23132

Steam candidate E1F7B06B64DA6B0EF5C57823C310AF85EBD3B3C4F22D87DB8C48BD449EF0B5F1,
source 7221829. MAIN launched Steam at the user's request, waited for gameplay,
focused OpenXR session5 and both tracked hands, then captured OFF and ClearGate ON
without restarting or replacing files. Rejected Receiver experiment stayed OFF.

Private traces shadow-pass-23132-1/2: OFF617 calls/202 groups, ON1409 calls/294
groups, both zero overflow. Views differ, so total pass counts are not an A/B
effect measurement. The same gun VB6F3B5940/5799 vertices is present in both.
OFF native shadow-map depth clear is after488, actual scene depth/stencil clear
after560. Gun late depth groups165/166 have fgCount90, moved0; late color
groups169/170 have fgCount93, moved0, versus early depth groups0/1 moved1.
The source rejection limit is64. This directly supports premature foreground
window exhaustion rather than merely inferring it from a counter aggregate.

ON gun late depth groups254/255 have fgCount1, moved1; color groups258/259 have
fgCount4, moved1. Early depth remains moved1. Thus the exact gun resource's late
depth/color ride classification is now consistent with its early depth pass.
Receiver first pair is phase0/count0 and late pair phase1/count6. Runtime reports
ignoredOffscreenR32 and allowed native scene clears; native stereo stays active.
Captured receiver alpha state is ZERO/ONE, separate alpha ADD: destination alpha
is preserved by blending, unlike the earlier unknown blend-op evidence.

This is real Steam/OpenXR execution evidence, NOT pixel-content or physical
visual PASS. No claim yet that square shadows, controller-following distant
shadows, or black/frozen gun materials are fixed. Physical feedback and verified
post-session restoration remain required. Bone palette contents are not proved
by moved flags. No matching world-decal comparison was established in this pair.

User feedback after this same-session test: stereo apparently OK, weapon tracking
OK, no remaining/trailing weapon shadow; shadows still square. This is a partial
visual result, not proof that every black/frozen material or distant shadow is
fixed. User additionally reports progressively falling FPS while holding L2 or
R2, recovering to90 when released. Ordinary-shadow preservation was not answered
separately. Game exited; guard removed ClearGate flag and restored accepted DLL.
MAIN verified all3 actual hashes: DLL9FBA308C..., INI61AEC56C..., UI5B2EA469....

Retained runtime log contains39.7FPS/25.21ms and later89.9FPS/11.12ms. Measured
OpenXR sync/actions/poses sum about0.18ms in that slow interval; haptics0ms,
copy0.01ms, submit0.27ms. "Our work"24.90ms is an unassigned residual, not proof
of mod CPU cost. Existing CPU buckets label all90Hz frames SLOW (FAST>=105),
so their cumulative table cannot isolate this regression. Trigger endpoints
and some whole reporting windows read zero despite slow frames: no established
press-duration correlation from this uncontrolled log alone.

Next bounded instrument: owned hotpath-gameplay.request arms the existing sampled
hook profiler only after focused tracked gameplay. One3600-frame window/process,
default OFF, no INI edits; rejected if INI profiling already enabled. Existing
thread sampler additionally prints non-cumulative CPU deltas during this window
(process user/kernel and Present-thread CPU), allowing burn-versus-wait comparison.
Trigger endpoints are explicitly labeled endpoints, not entire-interval states;
initial interval can straddle arming. Existing nested hook times are inclusive
and must NOT be summed as independent costs. No shader/input/weapon changes.

## Gameplay trigger test PID13296 / FD8C79A3, 23:00

User confirms manual exit, not crash, and completed physical L2/R2 holds. The
full runtime log does retain them BEFORE profiler arming: L2 sample17.8FPS,
LT255/RT0 plus impulse held58/120 frames; sustained R2 samples34.6/34.7FPS,
RT255 and physical min=max1.00. Both experiment flags OFF. Therefore this
symptom also reproduces without ClearGate; its cause is not established.
OpenXR calls/copy/submit remain small; unassigned residual must not be called
mod CPU or total GPU time. Present/gpu-copy spans are not total frame GPU cost.

MAIN armed the hook sampler too late (log line8988); all its retained120-frame
CPU intervals end with LT0/RT0 and most show menus. Full3600-frame report did
not finish before exit. This is INCOMPLETE_PROFILER_EVIDENCE, not absence of a
trigger defect. All3 restoration hashes verified. Next candidate retains
bounded cumulative partial hook reports every240 frames, with actual elapsed
frame denominator and explicit progress label. Prearm request before launch to
cover first gameplay instead of depending on a late log-poll/user notification.

## Full profile PID36504 / 76844456

Prearmed profile completed3600 frames with partial240-frame snapshots retained.
Physical user confirms L2 worse than R2 and release recovery; additionally Jump,
Circle, R1 and Square also produce FPS drops. Input-only cause is a hypothesis,
not a finding. L2 reporting sample17.1FPS; sustained R2 intervals20.304,
21.909,24.440ms, with RT255 endpoint and latter windows physical min=max1.00.
Released intervals return~11.12ms. Runtime CPU deltas in those R2 intervals:
process user+kernel15.494,17.057,14.193ms; Present-thread4.688,4.688,3.906ms.
These are sums across threads, not a serial frame budget. Mixed first interval
is not evidence of sustained L2 CPU cost.

Partial sampled inclusive draw-hook estimates~0.83-0.89ms/frame; pose/input~0.19ms,
submit~0.25ms, no multi-ms classification or palette spike in these aggregates.
Some stages are nested; don't add their times. Driver/native drawing outside the
measured scope and unsampled spikes remain possible. Per-thread deltas derived
from rounded cumulative logs for the two sustained R2 windows put the largest
reported CPU on TID9610, start Singularity.exe+0x01B542ED (~7.28/6.64ms).
Thread start address is NOT the hot instruction or function identity.

Game exited normally; DLL9FBA308C and UI5B2EA469 verified. INI changed only
LaserSight1->0 via in-game persistence; preserve it (new SHA256
568E2BB5674E88F9C0F24B28A33710866282CE44E5C0C18848B33F6F81FB75B8),
do not silently overwrite user state. No full3 rollback claim for this session.

Next instrument tools/record-trigger-cpu.ps1 uses available Windows WPR CPU.Verbose
with a unique named instance, refuses any existing recording, bounded launch
wait and90-second/process-exit capture. No game hooks, settings, input injection,
or DLL replacement. ETL is private and system-wide; analysis must filter game PID.
Needs explicit administrator/UAC authorization: current shell is not elevated.
Not yet run; no stack-level attribution until samples are captured.

## Bounded Windows CPU trace PID33004, 23:12

User authorized UAC and physical test on accepted DLL9FBA308C. Named WPR instance
started, independently confirmed through logman session inventory; stopped and
saved privately after90 seconds from game launch. Default non-elevated WPR status
did not expose the elevated named instance: it is not evidence that capture failed.
ETL2,969,567,232 bytes; Perfinfo422,680 system samples. Game-only filtered CSV
contains78,190 IP samples. Own stream analyzer handles actual I-Start/EndAddr
and PrgrmCtr schemas, verifies PID33004, and can retain unique frame-hit counts
for one sampled thread; these are inclusive counts, not additive CPU times.

Physical user: drops on L2/R2, Jump, Circle, R1, Square; normal game exit. Files
remain accepted DLL9FBA308C, UI5B2EA469; current INI returned to61AEC56C via
game persistence (no MAIN overwrite). Private read-only .text snapshot PID33004,
base00400000/RVA2000/size0138011A, zero failed pages, taken AFTER recording.
Raw code/disassembly and ETL never committed. Full stack totals include context
switches; CPU-only export must filter event "Sampled Profile" (with space).

TID10764 corresponds to the previously observed non-Present game thread,
start+01B542ED. Its hot004886BC IP belongs to a bytewise left-shift/table checksum
loop at00488690, but573/1022 hits are in trace second16 (startup), so NOT a
button root-cause finding. Avoid misleading global hot-address patching.

In later seconds59/60, sampled stacks include original platform input polling,
DirectInput forwarder and Steam Overlay. Accepted-map provenance and DLL SHA
confirmed against pause-native worktree: own DLL RVA3F916 maps to NativeVrUi::Poll
+0x26. Source forwards originalPoll before Drain; its presence in a stack does NOT
prove our UI bridge owns the sampled cost. Original native input virtual return
011D3903 confirmed by private assembly. Temporal button-edge matching and native
subpath identity are not complete; no input fix claim.

Next minimal experiment adds one existing-sampler family around originalPoll
inside the existing hook (no new native hook, no argument/result/behavior change).
Default OFF unless bounded gameplay sampler requested. Hypothesis: native polling
cost grows on held controls. Falsifier: native_input_poll remains small while FPS
drops, moving investigation to other game/update or waiting paths. Keep ClearGate
and Receiver OFF to isolate; inspect existing per240-frame partial profiles.

## Native input polling measurement PID36472 / 4DBF5C9B

Completed3600-frame window, originalPoll3727 calls/116 timed samples; estimated
native_input_poll11.5214ms/frame, sampled mean11128.809us/call. Partial720/960/
1200-frame estimates10.3271/13.0245/18.4084ms/frame, versus initial/loading
480-frame0.0905ms/frame (not a stable gameplay baseline). Native cost localized
inside originalPoll, not automatically DirectInput exclusively. User confirms
all test buttons still drop FPS. Released final intervals~90FPS. Game closed;
accepted DLL9FBA308C, INI61AEC56C, UI5B2EA469 restored/verified.

Further retained ETW stack at58311362us/TID10764 enters cfgmgr32 through DirectInput
and Steam Overlay under native input dispatch. Actual dinput8_orig matches the
installed Windows SysWOW64 dinput8 SHA2A746CCF...; not a modified renderer asset.
Loaded native helper011D2FC0 reads interface pointer01D1FC7C, calls vtable+10h
with type4, callback011D2180, usernullptr and flags1, then returns. This is the
DirectInput8 EnumDevices path for attached game controllers. Two static callers
invoke it conditionally while processing input; neither consumes its return.
The condition's UE3 field identity/runtime value is NOT established.

Next bounded timing probe exact-fingerprints all29 helper bytes, hooks the proven
no-argument entry, retains EAX return and original call/callback. Optional sampler
family native_gamepad_enum quantifies frequency and inclusive cost. No enumeration
suppression, caching, device disconnect or input filtering implemented yet.
Hypothesis: enumeration amplification dominates originalPoll under held input.
Falsifier: enum time/call counts remain small while originalPoll grows. This
extra native hook is justified by the existing-hook timing and actual stack path;
fails closed on unsupported loaded code. No guessed UE3 structure offset edit.

## Enumeration timing PID31948 / A0068C7A

Exact hook create/enable0/0 at Steam runtime. Completed3600 frames: helper309
calls,9 timed samples, sampled mean75266.567us/scan, estimated6.4604ms/frame;
parent polling3599 calls/112 samples, estimated4.8177ms/frame. Different sampled
populations explain why the child estimate can exceed parent: do not sum them
or claim an exact percent. Released intervals~90FPS. Native expensive scan path
is measured, not merely a pointer/counter finding. Guard restored accepted DLL;
MAIN verified actual final3 hashes9FBA308C/61AEC56C/5B2EA469 after this session.

## Default-OFF 500ms scan-throttle experiment

Owned input-enum-throttle.flag payload/20-minute lease. Eligible only for VR,
emulated pad ON, menu class0, and the exact thread latched by native input polling.
First scan runs natively; successful EAX result may be reused for500ms after its
completion. Failures never cached; other threads/contexts pass through. Native
helper/callbacks are NOT invoked on deferred calls. This changes device-scan
timing and may defer hotplug detection or callback side effects; neither is
claimed safe in production yet. No button filtering, pose/shader/weapon/UI edit.
Disabled state preserves native calls. Disable flag resets cache validity.

Existing hook is reused. Full-rate bounded-window enum timing replaces1/32 for
this rare75ms family; all other timers stay sampled. Native/deferred counts
reported once/second from Present, never disk logging in the input hook. Guard
removes owned experiment flag after exit/no-launch before restoring DLL.

Offline x86 tests PASS: native failure bits preserved, failure not cached, first
success native then defer, ineligible/other-thread pass through,499/500ms and
DWORD wrap boundaries, unsupported signature untouched. Real runtime/physical
test pending. Falsifier: scan count falls but input-poll/FPS do not improve, or
buttons/menu behavior regress. Use same-session OFF/ON while HQ Decals ON; keep
both shadow experiments OFF, restore accepted DLL after test. Do not mark fixed
from offline tests. If FPS improves but periodic scan hitches remain,500ms gate
is diagnostic progress, not final performance solution.

## Evidence, real Steam PID5260 / DLL6D147048

Focused capture 2: 1329 final calls, 322 state-qualified groups, zero overflow,
11 native clears, zero intercepted StretchRect calls. HDR depth copies instead
use PS 0df8d996... TheTexture. Resource identities and private bytecode retained
locally. Initial automatic capture was a loading-transition frame, not usable
scene evidence; no visual fix claim from either capture.

Same retained gun resources VB4A68AF00 / IB4A3CBD20, 5799 vertices, 6767
triangles: depth-prime groups0/1 are moved=1, late base-material groups293/294
are moved=0. Both shaders' CTAB identifies bones c5..229 and LocalToWorld c230.
Late base material has an alpha-depth output and enables its write, with near coefficients ~0.999,
while earlier world producers/receiver use ~9.99. Native Clear afterDraw1052
targets the R32F512 shadow map; afterDraw1280 clears scene depth/stencil before
the late first-person base pass. Some preceding non-skeletal decal materials
are moved=1. Movement flags alone do not prove bone palette contents.

Source Hook_Clear opens g_inForeground after ANY depth clear beyond draw50,
without checking its target. FgMatchWindow / FgMoved reject once foreground
draw count grows beyond a measured small-pass limit. This gives a falsifiable
mechanism: a shadow-map clear starts that window early, allows world effects to
ride the controller, and exhausts the classifier before actual gun color draws.
Exact draw-by-draw counts remain to be measured; no speculative register edit.

## One-variable experiment

Flag shadow-clear-gate.flag, independent of the rejected receiver flag, OFF if
missing and expires after20min. Only late offscreen R32F clears are excluded
from opening the foreground classification window. Native Clear still receives
all original arguments and returns the original HRESULT. No changes to shader
constants, weapon/hand transforms, tracking, UI, materials or shadow generation.
Atomic render flag/counters; no allocation, device query or disk write in Clear.

Added bounded read-only order: each final draw retains operation serial, group
and foreground count; copy/clear records share serial space. Blend operations
are also captured. The analyzer parses actual SM3 CTAB register metadata and
distinguishes alpha-write permission from destination-preserving alpha blend.
Older traces without blend-op data are UNKNOWN, not assumed ADD. Native captured
receiver CTAB validates ScreenToWorld c3/count4 and MinZ_MaxZRatio c2 against
private disassembly. Synthetic metadata/alpha tests contain no game bytecode.

Falsify OFF vs ON in one session, HQ Decals ON throughout: OFF should expose
the premature window / excessive foreground count; ON must ignore the measured
R32F clear, keep world decals unmoved and preserve the gun's material/depth ride.
If counts or pass identities do not support that, disable without expanding
the classifier. Physical gate: square/distant shadows, black/frozen gun surface,
ordinary shadows, stereo and accepted hand/menu appearance. Do not mark fixed
from counters or compile alone. Restore exact accepted3 hashes after session.
