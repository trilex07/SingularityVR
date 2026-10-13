# Bounded shadow-map content diagnostic

Remaining symptom: square shadows with HQDecalsON. Input FPS cache physically
passes; do not mix input changes or re-enable rejected receiver correction.
Earlier traces establish an R32F512 sampler read by candidate volume receivers,
but resource identity alone does not prove completed meaningful depth pixels.

Hypothesis split: corrupt/empty source shadow map versus receiver projection
or coverage. Read pixels immediately BEFORE the final driver volume draw using
that sampler, correlate exact group/shader identity afterward. This establishes
content at consumption, not a final-present image of a potentially reused map.
It does not by itself identify the square artifact's contributing pixels.

One explicit shadow-pass request, max4 distinct512x512 R32F render-target
textures at sampler1 on8-vertex/12-triangle draws. Staging1MiB allocated at Arm,
never bound; no shader/constant/texture/render-target/state replacements.
GetRenderTargetData uses same format/size SYSTEMMEM destination; reject MSAA,
wrong pool/usage/format/size. HRESULTs retained rather than fabricating content.
Lock/Unlock/COM references balanced. Statistics include full finite/nonfinite,
zero/one counts,min/max and16x16 center samples. No raw game pixels published.
Readbacks can stall the one requested frame; no continuous GPU readback.
Trace OFF still issues zero device queries. Limit is not an error or overflow.

SDK constraints: https://learn.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3ddevice9-getrendertargetdata
Synthetic x86/O2 stats tests verify padded row pitch,NaN,min/max/grid and invalid
bounds. Existing trace OFF/COM/shader-size tests retained. Real driver readback
and physical shadow outcome pending; no fix or pixel-validation PASS yet.

Test next: same scene/HQDecalsON, stable head/controller pose, input-cacheON,
ReceiverOFF/ClearGateOFF first. One focused captured frame. Match readback group
to audited receiver shader; if pixels valid inspect map content and actual
producer/consumer transforms, not guessed camera constants. If readback fails,
preserve its HRESULT and choose a supported read-only resource capture route.

## Real Steam readback PID18352, 2026-10-13

DLL4EADB827/source3b262e8. Trace1769 calls/370 groups/overflow0. Before draw1470
group254, sampler1 texture2CA6BC20: HRESULT0,262144 finite values,0 nonfinite,
22405 zeros,0 exactones,range0..2.71828198. Group matches exact audited receiver
VSd09ad82c/PS28e01133. Content-at-consumption PASS for that resource; not proof
of physically correct depth/shadow coverage or all lights. One unique map read.

Raw source2CA6B520 producers248/249 have native512 target,viewport6,6,272,178,
full512 scissor. Copy250 PS0df8d996 and filter251 PS3b93fac6 alternate before
receiver. Private disassembly confirms filter has internal constant~e and
clamped arithmetic, receiver performs exponential/log shadow comparison.
Therefore values>1 up to~e are NOT by themselves corruption; do not replace
them with normalized hardware-Z assumptions. Retained shader metadata/instructions
and map statistics distinguish actual data from a successful bound pointer.
First snapshot does not cover later reuse of same map for another light.

Physical user says diagnostic leaves image/controls unchanged. Clarification:
"put away" means the mod's VR holster action. The observed shadow stays at the
position it occupied when holstered; head movement changes only the observer's
viewpoint around it. Square shadows are visible around enemies, implicating
dynamic casters as a separate or shared source. This is visual evidence, not
proof of the responsible map, caster draw or matrix. The holster code latches
weapon orientation and moves the weapon mesh to a body-relative hip transform;
it does not intentionally latch any shadow map. Falsifiable hypothesis: the
weapon/controller transform reaches a dynamic shadow caster or receiver, and
the apparent frozen shadow is its last map content or a cached transform. A
trace in which that caster/map follows the live controller after holstering
would disprove the proposed latch. Next controlled capture: same enemy scene,
HQDecalsON, stable player/head position, weapon armed at two right-hand poses,
then mod-holstered while the right controller moves. Compare source map
content, producer transforms, receiver constants and event order by actual
shader identity; COM addresses are session-local. Keep both shadow correctionsOFF.

After exit guard084305 removed input flag/restored DLL9FBA308C. Actual DLL,
INI61AEC56C and UI5B2EA469 separately verified. No fix claimed.
