# World-depth receiver reconstruction experiment

Observed in physical PSVR2 PID35208: live enemy and square shadow visible.
One pause-return capture:1000 calls/266 groups/overflow0. Enemy6481/4307 mesh
VB2F99DDA0/IB2EF663C0 produces512R32F at viewport6,6,418,368; producer181
successfully records LocalToWorld and ProjectionMatrix. Receiver186/187 uses
the audited VSd09ad82c/PS28e01133. First scene-depth parameters are near10
(MinZ_MaxZRatio9.98999977,.000999987125,.1001001,.000100098812).
Later arm map reuses the same textures; do not confuse first and tail constants.

Actual shader instructions prove VS c0 ViewProjectionMatrix writes identical
clip position and PS varying. PS reconstructs [clipX,clipY,linearW,1], then
uses ScreenToWorld c3..6 and ScreenToShadowMatrix c7..10. The VS matrices differ
per eye/SBS; both native PS inverse matrices are identical. Native light-map
chain is coherent: canonical ScreenToWorld * actual producer ProjectionMatrix
* viewport/half-texel/depth scale agrees with ScreenToShadow to max~0.000114.

Falsifiable mechanism: SBS/per-eye clip XY are consumed using the native mono
inverse, so world/light coordinates sample outside the correct shadow map.
Offline arithmetic at the caster LocalToWorld origin reconstructs world with
523.156/554.729UU error and shadow UV(.2498,3.3943)/(.7020,-2.3103). Inverting
the actual [VP.colX,VP.colY,VP.colW,affine1] maps that sample back with~1e-13
error and both eyes sample UV(.482658,.403195). This is a mathematical sample,
not pixel validation. It assumes scene alpha decodes actual cameraW.

Smallest experiment: separate default-OFF flag, exact audited shader hashes,
scene HDR/R32F resource shapes, half-eye scissors, non-controller-transformed
volume draw and captured world near10 depth family only. Canonicalize the
unused W column (PS reads XYZ only); derive actual per-eye inverse; preserve
native world-to-shadow by rebasing c3..10. Upload only for that final native
draw and restore all original eight constants afterward, including failed
partial uploads. Fault-latch if restore fails. No first-person near1 receiver
correction; old rejected experiment remains off. Counters alone are not a fix.

Disproof: confirmed application to the world receiver without improvement of
enemy squares, loss of normal shadows, or any stereo/geometry regression.
Then disable the new flag, preserve OFF/ON traces and inspect scene alpha at
consumption. Do not repeat the prior unbounded world/first-person receiver fix.

Implementation L1: synthetic x86 /O2 test passes default-OFF/no-query behavior,
world/first-person and moved-draw exclusion, both-eye camera round trips,
unused-W canonicalization, balanced COM refs, scoped eight-register restoration
and partial-upload/restore failure handling. No runtime or visual pass yet.
`set-shadow-world-experiment.ps1` controls the separate 20-minute owned flag;
the deployment guard's `-ShadowWorldReceiverExperiment` removes it on exit.
No proprietary captures/assets are shipped. Only user-requested saved setting
ShowControllerArms1->0 changed after PID35208, restoring gloves-only behavior;
the experiment does not alter hands or weapon placement.

L2_PASS: source532cf448aff4cf72398177eb0ea45acc21e54915, x86 PE32 /O2
Performance build SHA7A7787EBB2B41CE0B54F94223FFC48A390A6A650FB2CC6E506036D9E3DDB6FAE.
Build manifest matches the actual DLL; no tracked source edits during build.
Trace parser regression, runtime-policy checks and flag/restore-guard parsers
pass. Not deployed or visually verified at this gate.

## Steam and physical result PID37880

MAIN deployed the verified DLL7A7787EB via the runner, input cacheON,
WorldReceiverON, rejected ReceiverOFF, ClearGateOFF. INI C59B1CF6/UI5B2EA469
unchanged. Real Steam/OpenXR initialized. One pause-return frame contains1342
draws/314groups/overflow0. Dynamic6481/4307 producer228/229, viewport6,6,472,342,
feeds audited world receivers234/235. Final GPU constants reconstruct the caster
origin with3.05e-5/6.60e-6UU error; both shadowUV~(.399054,.246867). Light-map
chain still agrees (residual0.000465/0.000446). Later near1 inverse is identical
in both eyes/native and skipped by the experiment. No restore fault reported.
This proves scoped constant execution, not per-pixel depth correctness.

Physical user explicitly confirms enemy shadows work and are dynamic. This
tested-scene result is PHYSICALLY_VERIFIED; general lifecycle/other scenes and
normal-shadow/stereo acceptance were not separately reported. Black/frozen gun
and controller-following scene-negative artifacts remain BROKEN (also previously
seen in accepted build). Do not call the entire shadow/weapon system fixed.

Actual gun5799 VB69D3E700/IB2F58BD20 early depth groups0/1 are moved1. Same exact
resource late depth297/298 has fgCount101/moved0 and late color301/302 has
fgCount104/moved0, beyond64 rejection limit. Scene depth clear after1309 does
not reset the already-open window; earlier shadow-map depth clear opened it.
This matches the separately measured PID23132 ClearGate mechanism/result.

Next one-variable experiment: retain WorldReceiverON and input cacheON, enable
only existing ClearGate. Prediction: late gun depth/color movement aligns with
early depth, scene effects stop being misclassified as foreground, enemy
reconstruction remains active. Disproof: counts/movement remain inconsistent,
gun/background remain broken despite aligned movement, or enemy shadows regress.
Do not expand shader constants into near1/first-person or alter hand transforms.

After PID37880 exit, guard removed both owned flags and restored DLL; MAIN
independently verified DLL9FBA308C/INI C59B1CF6/UI5B2EA469. Snapshot111315-949,
runner backup111316. Current installation safe, gloves-only setting retained.
