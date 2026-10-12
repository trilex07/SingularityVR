# Native shadow-depth clear classification experiment

Status: IMPLEMENTED_NOT_VERIFIED, default OFF. No shader correction.

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
