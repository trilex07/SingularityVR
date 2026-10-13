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

## Verification and next measurement

Guard source f57c3d5: six x86/O2 test executables and runtime policy PASS;
regular x86 PE32/O2 DLL64353686EDD457EDE84B0D8892B32B6EC9D108C78912AEF9A7F753CFA163DC15.
No Steam/physical validation yet. Immutable candidate remains offline while
the accepted installed DLL stays D892.

The optional record-trigger-cpu.ps1 -AfterCinemaExit path waits for the fresh
owned CINEMA_EXIT_HOTPATH armed marker from this exact PID/start time (max180s),
then starts its uniquely named Windows CPU stack recorder. Existing WPR/ETW
sessions are checked again after waiting. Max90s trace planned, stop own
instance only, never mix a restarted PID. No guessed movie-duration trigger.
This requires a fresh explicit user UAC authorization; syntax is checked
offline but recorder execution/ETL evidence are pending. Raw system-wide ETL
remains private; analysis filters Singularity only.

## Follow-up session PID8804 — physical performance remains broken

Steam launch19:18:09, regular guard DLL64353686 exact installed hash verified,
original UI/DLL/INI backups verified, current user INI preserved. Guard3472
used DLL-only backup20261013-191743 (one file, zero directories). Read-only
observer12956 captured existing log transitions and full session after exit.
Physical feedback: hands still stretched; FPS drop at catastrophe; influence
of buttons unknown. Do not mark the second-entry crash repaired from this
session: no explicit normal second New Game acceptance or contained-fault
execution has been established.

Native cinema initially~90FPS, then4.2 and2.2FPS, actualOFF and stereo return,
then~6FPS. After-exit hook profile45,015ms/265frames, idle227/LT26/RT12,
menu0/cine0. Sampled Present-hook2.818ms/frame versus~167ms native frame;
enumeration0, cached callback replay0.846ms/frame. This is a large unexplained
outside-hook/inclusive timing gap, not evidence for another enumeration patch.
Pose updates can be~0.4-0.5s stale; cannot infer mesh-stretch cause from that.

User explicitly approved UAC. Recorder attempts4548/26980 terminated before
tracing: live ReadAllText sharing conflict independently reproduced in Windows
PowerShell5.1 while native log writer was active. Corrected Get-Content shared
reader tested live; ordinary transient I/O becomes a missed poll, fatal errors
are transcribed before cleanup. Pure fresh/appended-cue tests PASS.
Recorder22032 then confirmed actual cinema-exit gate, uniquely named own WPR
instance; ETL saved successfully, stop/merge completed19:24:10. Capture up to
90s or process exit; don't claim full90s before reading trace timestamps.
ETL2,501,902,336bytes stays private; CSV export and Singularity-only stack/IP
analysis in progress. No unrelated process data or raw trace in commits.

After normal game exit, independent hashes verify safe installed DLL D892,
user INI B3FEED43...2A079F and UI5B2EA469...673CDE. No game remains running.

## Native trace identifies texture hashing, not the cinema composer

Complete export24,659,702,027bytes, private. Target-only sample/image/Present
stack selection501,046,124bytes; corrected analyzer understands actual xperf
one-frame-per-row Stack schema (No., Address, Image!Function), retaining full
caller frames, excluding ordinals/duplicate frames. Legacy/current synthetic
schemas and PID filtering PASS. Final target-PID analysis111319samples;
Present thread30652 has78175samples. Module detail target-weight total111329078,
texture injector72119764 ->64.78% of target weighted CPU samples, NOT4.39%
(the latter denominator is all system CPU). CPU sampling is not GPU/off-CPU
measurement and stack-frame hits must never be summed as inclusive times.

Installed texture injector and SIDE source-built artifact both hash
AAA07005F4FF658C4A71CC30EA258E469DAF250C77BFE447011FFD650379E4F7,38400bytes.
Exact installed DLL disassembly locates top sampled RVA0x24E5 in mip0 hash
byte XOR/multiply loop, seed0xCBF29CE484222325 and prime0x100000001B3;
RVA0x5670..0x56A0 is the called64-bit multiply. Top IP0x24E5 has25174samples,
0x5698 has8090; these are actual executed producer instructions, not an
inference from the module name DirectInput. Our bounded enumeration counter0
does not measure this separate SetTexture interceptor.

Read-only source audit: texture_injector.cpp HookSetTexture hashes candidate
mip0 only on cache miss; classification cache512entries, on saturation CacheAdd
always replaces the FIRST entry without a replacement, not a rotating oldest
entry. Thus two uncached textures can evict the same slot forever while old
negative entries occupy the rest. Current installed config enabled1/apply1/
capture0/2x, all groupsON. Injector log35,231,921bytes contains533291 candidate
hash lines,655 distinct hashes; top repeated content appears75841times.
This supports saturated negative-cache churn and excessive hash/log work.
Resource identity/occupancy are not logged, so don't claim every repeated hash
is the same COM object or that no other cost exists. Falsifier: bounded cache
occupancy/miss/eviction/resource probe shows hashing while cache never fills,
or a preserving cache repair executes but the slow phase remains.

No modifications to installed injector/config/DDSs, SIDE worktree, accepted
hand/shadow/input systems. Next repair belongs to the separate frozen texture
injector, not another cinematic matrix/exit guess. Obtain explicit user scope
for a cache-only repair preserving target hashes, texture quality and asset
files before editing/deploying it. Mesh stretching remains open; current
ShowControllerArms1 preserved. User's button-duration effect is unknown.
