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
