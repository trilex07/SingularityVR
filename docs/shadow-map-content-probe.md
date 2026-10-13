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

## No-tab-switch holster protocol

The later PID35192 attempt recorded A (1137 draws/282 groups/overflow0,
R32F readback HRESULT0) but B stayed pending when switching to the chat caused
the headset/game to lose the focused, tracked gameplay gate. The player reported
the game freezing on tab switches. It closed, the automatic guard restored the
accepted DLL, and MAIN rechecked the DLL/INI/UI hashes before removing the one
owned pending trace request. Do not ask for replies between captures again.

Offline comparison of two real Steam traces gives a firmer caster identity.
PID35192 group2/3 draws a 3314-vertex foreground mesh with VB29A71EE0 /
IB0CA98E20 into the scene target. The same VB/IB is drawn by group257 (3314
vertices) and group258 (390 vertices) into a native 512x512 R32F shadow target
with viewport6,6,500,259. Both report phase1/moved0; this field alone does not
prove their vertex constants are identical or that the mesh is untransformed.
PID18352 independently repeats the exact same within-session VB/IB linkage:
scene group2/3 and 512x512 shadow-target groups345/346. Thus first-person geometry
participates in a dynamic shadow producer path. Later runtime logs identify
3314 as the arm-role mesh and5799 as the gun; the earlier gun label is corrected.
Whether this arm caster causes the frozen
shadow or squares near enemy casters remains unresolved; compare actual matrix
registers and content across controlled hand/holster frames.

`tools/run-shadow-holster-sequence.ps1` schedules three one-frame requests
externally; it sends no game input. At the enemy scene the player uses the mod's
VR holster once and draws the gun again. The `DRAWN` log event synchronizes A
at +4 seconds and B at +16 seconds. The player holds pose A for about 10 seconds,
moves the right hand and holds pose B for at least 12 seconds, then stows via the
same VR holster and moves the right controller. The `STORED` event synchronizes
C at +4 seconds. The player remains in PSVR2 throughout and closes the game
after another 15 seconds. Each request still requires 15 focused/tracked frames;
the runner rejects early stowing, timeouts and any pre-existing request. The
restore guard retains ownership of DLL rollback after exit. Compare enemy-map
producer and receiver data across A/B/C before inferring any causal fix.

## No-tab-switch Steam PID27084

The DRAWN sync worked. A was captured at931 draws/274 groups and B at919
draws/256 groups, both with overflow0 and successful finite R32F readbacks.
The identical within-session VB29904FE0/IB0CA5A540 was present in each
foreground first-person draw and native512x512 shadow target. Producer group249
in A and231 in B uses the same vertex shader SHA c69b26e9..., whose CTAB
names BoneMatrices at VS c5..229, LocalToWorld c230..233 and ProjectionMatrix
c234..237. The captured c5..31 rows vary between A and B; this is a measured
input difference, not proof that controller motion is its only cause. The
producer viewport changed from6,6,500,235 to6,6,500,280; this is not by itself
evidence of the square's cause.

The first sampled map in A/B instead follows a different producer: source
groups178/179 and164/165 draw 6481/4307 vertices into the same native R32F
target, then copy/filter before receiver groups184 and170. Within each trace,
their VB/IB also matches HDR scene groups150..153 (A) and corresponding B
groups. This distinguishes that world/dynamic-mesh route from the later
3314/390-vertex first-person shadow route. The source viewport is almost unchanged
between A/B (394x224 versus395x225); sampled maps both have 262144 finite
values but different zero counts (15413 versus8805). That difference alone
cannot locate square pixels because the light/subject may move during the
physical sequence.

The player reports that enemy-adjacent squares did not change with right-hand
motion, while the gun-associated shadow stopped at its previous location after
VR holstering. These are distinct observed behaviors. C has no trace: the
40-second STORED wait expired immediately before the user's holster event; a
manual request was issued afterward, but the game closed before consuming it.
The request was removed after exit and the accepted DLL/INI/UI hashes verified.
The scheduler now permits a120-second per-stage wait. No need to repeat A/B
merely because C was missed.

Next bounded producer probe reads VS c230..240 only when a new native512x512
R32F non-quad draw group appears. These register names come from the captured
shader CTAB, not guessed UE3 offsets. The prior trace stored c0..31 only, so
it cannot distinguish the gun's local-to-world/phantom position from shadow
projection changes. One extra read per qualifying group preserves the original
render state and records HRESULT. Compare c5..31 and c230..240 for the exact
first-person VB/IB at armed and holstered poses; analyze enemy producer separately.
Source dbce0ed built x86 PE32 /O2 with performance diagnostics, DLL SHA-256
54CC148FB994E2906634AD522A45597F71F4E22702B91993C97A73EEDE91B787.
Python parser synthetic check, C++ trace OFF/COM check, shadow-depth stats and
runtime policy checks pass. This establishes L2 only; new c230..240 records
were subsequently validated in PID12648 below.

## Producer transforms in real Steam PID12648

DLL54CC148F/source dbce0ed, one focused trace2304 calls/351 groups/overflow0.
The holster scheduler never synchronized initially. MAIN stopped only that
waiting helper and requested a single frame; after returning to gameplay the
request completed and produced an XR-backbuffer capture at09:51:18. The player
later succeeded in using the holster, but those events occurred after the sole
trace. They report disappearance of controller-following shadows, persistent
enemy squares, a black gun and darker textures. This is not visual acceptance.

Runtime latch logs in PID12648 and PID18352 explicitly name gun5799/arms3314.
This corrects the prior label of3314 as gun. In PID12648 the actual5799 gun
VB69937F40/IB526B3100 is moved1 in scene groups0/1,327/328,331/332 and is not
observed in a512R32F producer draw. The3314 arm-role VB29A9B960/IB0C4426A0
links scene groups2/3,
329/330,333/334 to shadow producer336, with390-vertex companion337. All these
arm-role draws report moved0. Resource linkage alone cannot establish the mesh
role; use the runtime role evidence when interpreting old traces.

Groups336/337 successfully captured c230..240 and share LocalToWorld and
ProjectionMatrix. The map viewport is6,6,500,245. Receiver342 is the audited
VSd09ad82c/PS28e01133 and consumes the filtered512R32F map after copy/filter.
Readback before draw2291:262144 finite values,0 nonfinite,19894 zeros,range0..e.
Private disassembly proves the caster uses skinned position -> LocalToWorld ->
ProjectionMatrix and stores exp(saturate(clipZ*InvMaxSubjectDepth+DepthBias)).
The receiver decodes linear viewW from scene alpha, transforms that screen/W
vector with ScreenToShadowMatrix, and compares against this exponential map.

`tools/check-shadow-matrix-chain.py` checks an explicit caster/receiver pair
using audited shader hashes, CTAB ownership and row-vector instructions.
For336->342 the inferred texture scales are X~0.488281956,Y~-0.239257858,
depth~0.001024079, consistent with500/1024,-245/1024 and captured inverse depth
0.001024072. The proposed viewport plus half-texel chain has max coefficient
residual~0.195865, concentrated in translation; inferred Y bias differs by
~0.0002365 (about0.12 shadow texel). Do not treat that residual as a proven bug
or patch it by fitting constants. This does not validate actual per-eye depth
reconstruction. This frame has no confirmed active-enemy shadow producer.

User changed ShowControllerArms0->1 and RecenterChord0->1 after the black gun
appeared; a VR Settings save also wrote existing defaults. MAIN preserved that
INI (B3FEED43E7651E86C57B3767359F13C363B2F3BBFE067429E29F81F4CB2A079F).
Guard restored DLL9FBA308C and UI5B2EA469, both independently hashed; no owned
flags or pending requests remained. Next live-enemy measurement can use the
external scheduler's `-Mode PauseReturn`: actual submitted class3(SPPause) ->
class0(gameplay) schedules one request at+1second. Runtime log contains three
such transitions; parser checks pass. This avoids both the holster dependency
and chat switching. Keep the existing read-only DLL candidate; gather the
active-enemy caster matrices before further rendering modifications.

Player clarification: the enemy in PID12648 was already dead. They also see
square shadows under living/moving enemies, sometimes intersecting the body.
Thus caching dead-body maps is only a possible contributor, not a sufficient
explanation. The sole trace has exactly two audited ShadowDepthTexture receiver
groups342/343, both reading the arm map512R32F; it contains no other R32F sampler
and no confirmed enemy-caster map. Do not attribute enemy-square pixels to the
captured arm map. Next measure a live enemy before firing, using pause/resume
as synchronization. The snapshot runner accepts an explicitly selected current
INI SHA via -ExpectedIniSha256 so user changes can be backed up and retained
without weakening the DLL/UI hash checks.
