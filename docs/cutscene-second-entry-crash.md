# Second-entry camera crash — 2026-10-13

Observed on CPU-profile candidate D430CE9F1607BF0F4B55639196E90052A1066485F0AEAF7183D3B2C43341A036,
source 3bccd6746b4978dc6715866e8d06da73fa812d6e, Steam PID14600.
User selected Continue (catastrophe cinematic), entered stereo gameplay,
observed stretched hands and worsening FPS, returned to doubled flat main
menu, then selected New Game and crashed. Do not claim either hands or menu
are explained solely by this access violation.

## Proven crash path

The first WER exception is c0000005, proxy d3d9 RVA0x1F6B8 (PE timestamp
0x6ACE61B0), reading address0x77630034. Exact immutable DLL disassembly and
MAP place it in FindCamera: `mov eax,[edi+34h]` after Readable(camera,0x700)
returned true. The address identifies cached camera0x77630000 and the
already-audited UObject::Class offset0x34. The saved dump CONTEXT is in the
exception handler, not the fault site; no original-register or unwound-stack
claim. The subsequent e06d7363/KERNELBASE report is not the first failure.

Hypothesis: Readable's cached positive mapping cannot protect a later camera
dereference across level allocation changes. Exact fault and field are proven;
unmapping timing and possible concurrent cache replacement are not. Small
controlled repair: snapshot the cached pointer for validation/use, guard the
whole existing lookup against memory access/in-page faults, discard the camera
and this thread's positive mapping cache on failure, return unavailable via
the existing caller path/backoff. No broad exception swallowing or renderer,
input, hand, shadow, asset, INI, or cinema-framing changes.

Falsifier: a further crash at this guarded lookup, or another transition crash
outside it. Callers' subsequent raw object accesses and live-object selection
remain separate risks; this guard is neither an atomic graph snapshot nor
proof that a readable object belongs to the current level.

## Performance evidence (separate, unresolved)

Actual after-cinema window:45,000ms,3474frames; input frames idle2456/LT917/
RT101/both0, menu0/cine0. Native log worsens90->49FPS during the window,
then about40FPS afterwards. Native Present-thread interval CPU grows roughly
7->19.7ms/frame. Sampled inclusive hook averages: native input0.273ms/frame,
cached gamepad callback replay0.200ms/frame, one enumeration12.46ms total,
draw1.743ms/frame, Present hook4.673ms/frame. Those averages are not additive
CPU totals, GPU times, or attribution of the unexplained native frame cost.
The old repeated76ms enumeration cause is not established here. Need a
representative slow-window native CPU stack profile before an FPS patch.

Observer terminated on a transient two-process observation; no full live
coverage claim. The authoritative full native log was copied after exit.
Guard restoration independently verified: DLL D8928180262706A95972D13B728A1867F1EAB1757DCFD6C4A32D99EFBFBEC1BE,
user INI B3FEED43E7651E86C57B3767359F13C363B2F3BBFE067429E29F81F4CB2A079F,
UI5B2EA469A3EE06FF154AC78BD58604E8034970D4837F289E5C22503C6A673CDE.
Raw log/dumps/game assets stay private, outside commits.
