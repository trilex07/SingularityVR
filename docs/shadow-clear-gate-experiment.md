# Native shadow-depth clear classification experiment

Status: IMPLEMENTED_NOT_VERIFIED, default OFF. No shader correction.

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
