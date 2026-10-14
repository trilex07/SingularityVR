# MAIN resume: round 3, no mid-session chat

## PID45372 / 7A0EC94D: camera and cinema exit physically passed; CPU/hand gate open

User confirms image OK, head no longer rotates content, actual stereo returns
after crash/get-up. Reports crash~20FPS, gameplay~40FPS, hands do not track pads
properly, heldL2 drops~10FPS. Helicopter PL dialogue remains unimplemented.
Full private evidence `deployment/cinema-native-camera/test-live/session.log`:
native-camera60% executed; helicopter~90FPS; second cinematic OFF pair at
15573-15575, GameplayStereo at15587; later pause/resume transitions recorded.
Controller state stays PlayerWalking while both cinematic ON and OFF: do NOT
promote that state-name observation into an exit override. Native OFF works.

Some measured native windows6.7FPS/148.98ms; cinema-copy CPU~0.02ms (enqueue
scope, NOT GPU completion). One-second thread delta during slow phase: Present
TID30652/77BC about1046.875ms CPU/1005.956ms wall; driver TID38480~140.625ms.
This gives a render-thread CPU lead, not an exact function/stack diagnosis.
INPUT_ENUM_SCAN later has native0 with replay4..24: accepted topology cache
executes. Do not assume discovery again causes L2 stalls. Possible callback
replay versus draw/state/palette CPU costs require direct measurement.

Guard restored D892 and UI5B rehashed. INI changed only ShowControllerArms0->1
via VR panel persistence at20544; SHA B3FEED43E7651E86C57B3767359F13C363B2F3BBFE067429E29F81F4CB2A079F.
Preserve this user change. No claim that INI reverted toC59. No hands/control
algorithms changed by MAIN. Camera/framing and intro accepted; do not perturb.

Next read-only experiment (user authorizes one more physical test): original
existing-hook sampler, explicit owned cinema-exit request, requires observed
CutsceneFlat followed by nativeOFF + GameplayStereo + focused tracked gameplay,
not initial menu or movie. Bound45s or3600frames; default OFF without request.
Measure cached callback replay as a SEPARATE family (all calls timed), preserve
callbacks/results/native-call behavior. Existing draw/state/palette scopes are
sampled1/32. Report actual measured frames even on timeout; no summing inclusive
families into a fictitious CPU total. Input LT/RT bins distinguish captured
presses from assumed choreography; pause/cine contamination counted. No new
input/render route, shader, game setting, asset or Polish audio change.
Hypothesis: a measured existing CPU scope accounts for the saturated render
thread. Falsifier: scopes collectively do not account for it -> actual Windows
CPU stack capture is next, requiring fresh explicit UAC authority. Do not guess.

## PID45080 / 8B34C584: intro accepted, cinema gate REOPENED

User confirms Polish intro. Cinema still too large/wide; turning the headset
rotates the content/shadows. After crash sequence, cinema/hiding remain active,
several FPS and no tracked hands. Full private native log:
`deployment/cinema-handoff/test-live/session.log`. Guard restored D8928180;
MAIN independently verified DLL, INI C59B1CF6 and UI5B2EA469. No assets changed.

Actual log contradicts a simple never-received-OFF diagnosis: SetCinematicMode
OFF twice at lines10222-10225, then ON twice at10229-10232. Helicopter windows
~90FPS precede the later16.3 then2.3FPS window (439.29ms). Existing `our work`
438.67ms is a residual INCLUDING rendering, not a measured mod function. XR
wait/submit, copy and object walking do not explain that interval. Do not label
the second ON stale, invent an exit timer, or claim stereo return fixed.

Source proves flat draw duplication stops, but SetVSConst still applies HMD
rotation/position/roll and VR projection, CalcViewRotation still overwrites
pitch, and ApplyVrFov still forces the cinematic camera's FOV. Hypothesis:
preserve native camera/projection in *only* CutsceneFlat so all native depth,
shadow and geometry consumers agree; continue to place the whole completed
image in a smaller60% identical-eye fit. Falsifier: native-camera gate executes
but content still follows head -> identify remaining writer, do not invert signs.
Gameplay, menus, Bink50%, accepted shadows/input and Polish intro are unchanged.

MAIN read-only Steam metadata boot PID36660 on D892: UProperty controls reproduce
Actor.Location54/Rotation60; RvPlayerController.mCinematicData is StructProperty
778,size24. Its mState IntProperty0 is a weapon-raise/lower state, not a universal
cinematic flag (enum PCS_STATE_INACTIVE/INIT/LOWER_WAIT/RAISE/RAISE_WAIT/RAISE_DONE).
No bCinematicMode property found: do not import an offset from another UE build.
Live UObject.GetStateName UFunction::Func+AC ->004B7B60 native instructions prove
self+18 ->FStateFrame, frame+28 ->StateNode, StateNode+2C ->FName. Read-only chain
returns PlayerWalking at boot. Native entry offsets are evidence, NOT hooks.
Next bounded probe validates StateNode's class `State` before naming it and only
OBSERVES current-controller identity/state; it must not drive an unproven exit.
Metadata boot closed normally via exact PID WM_CLOSE; no install writes.

Next candidate adds bounded per-second composition CPU timing and actual
controller-state transitions. Physical exit/performance gate remains open.
Priority from user: scene and start of gameplay first; full Polish dialogue/movie
coverage next. Helicopter speech remains unimplemented and must not be called PL.

L2_READY candidate: sourcef83fd71513a7792f639acfbcf7c861dacdabc6d7 (pushed),
regular x86 PE32/O2 SHA7A0EC94D22FDB1E27EFA5CA2469F28D1598126BFC5CF97004084B88AA02C1E65.
Eight narrow C++ unit executables PASS, native-state NOACCESS fault gates PASS,
runtime policy/PS syntax/diff check PASS. Shared pixel/MRT rollback GPU test
covers640x360/5264x2684 at85%;60% geometry separately covered by exact-fit unit
test. No physical performance/native exit PASS inferred.
DLL-only deployed18:19:34, accepted restore backup20261013-181931 (one file,
no directories). Guard29140/observer43780 READY,600s launch lease; original
DLL/INI/UI backup hashes verified, installedINI/UI unchanged. Game not running.
Next route without tab switching: New Game (intro may skip) -> whole helicopter
and getting-up sequence -> return to stereo/hands/pause if possible -> close.
If low-FPS discomfort recurs, stop promptly; no repeated long low-FPS trial.
Observer keeps full session log, explicit state names and bounded compose timing.

Inherited candidate, no renderer changes in this resume:
`fix/cutscene-mono-presentation` @ `3df65c843cf3b4aa1b6f223d5527cf68c809ee2a`,
x86 PE32 /O2, DLL SHA256
`072ED34ECEDD29E18E7BB895A4D512C73ADEEA168DB5C2A0572B2CD2DF6F6FED`.

User explicitly says round 3 has NOT been tested. Earlier new-game report:
intro videos too large/close; helicopter lacks cinematic presentation,
~20 FPS, visible tracked gloves/gun, movement possible, head yaw reversed.
These are separate unverified mechanisms, not one presumed compositor bug.

Hypothesis inherited from round 2: Matinee camera changes clear the cinematic
latch despite pawn continuity. Candidate preserves it across camera cuts.
Falsifier: latch stays ON but CutsceneFlat/composition does not engage, or the
same bad image remains with composition engaged. Rendering success logs do
not establish correct pixels, cinema depth, or head direction.

L1 rerun: mono-fit/predicate and scene-mode unit executables PASS.
Installed DLL equals inherited artifact/manifest; INI `C59B1CF6...` and native
UI `5B2EA469...673CDE` unchanged. Independent restore guard returns only DLL
to verified merged-main baseline `D8928180262706A95972D13B728A1867F1EAB1757DCFD6C4A32D99EFBFBEC1BE`
after observed exit or no-launch timeout. No process killing, runtime/registry
change, save editing or window/input automation.

Physical route: new game -> intro videos -> helicopter -> first gameplay.
Stay in PSVR2 until closing game. Observe whole image/proportions in each eye,
head direction, gloves/gun, FPS, then stereo/control and pause/options/resume
after transition. Abort if uncomfortable. One feedback report after exit.

`tools/watch-cutscene-session.ps1` observes existing logs only for a bounded
window; tail timestamps are observation times and can miss bursts. Final
session log (max 32 MiB) supplies complete transition evidence. It cannot
measure GPU time or establish physical acceptance. No new diagnostics flags.

## Crash gate: 2026-10-13, PID 41200

User: crash immediately after New Game, before the introductory video.
WER: access violation in candidate d3d9.dll at RVA `0x2898C`, PE timestamp
`0x6ACE167F`; exception record reads `0x0DA0A05C`. Candidate MAP locates this
inside Hook_Present; exact binary instruction is `mov eax,[ecx+0x5C]`, following
loads of `g_playerPawn`. Log acquired pawn `0x0DA0A000`. This is the Z component
of Actor.Location (+0x54), in camera translation stabilization. No CutsceneFlat
entry or compose log occurred. The dump's saved CONTEXT is in the exception
handler (ntdll), not the fault instruction; do not label those registers as
original fault registers or stack words as an unwound call stack.

Hypothesis: cached Readable predicates do not protect the subsequent pawn
sample when a level transition releases/reprotects the old pawn. The fault
site and object address are proven; exact unmap timing is not. Small experiment:
copy the already-audited location/eye-height sample under a memory-fault-only
SEH boundary, leave output unpublished on failure, clear only the invalid pawn
cache and skip stabilization for that frame. Keep camera math and all accepted
render/input/menu fixes unchanged. Falsifier: another crash at this protected
sample, or New Game remains broken elsewhere. L1 must exercise invalidated
and partial samples, valid math, unchanged outputs and unrelated exceptions.

Restore verified on disk: DLL D8928180..., INI C59B1CF6..., UI 5B2EA469...673CDE.
Full crashed session and dump preserved privately under deployment; neither
dump nor game assets are committed or transmitted.

L1 guarded-reader test PASS: valid location/height and stabilization math;
NOACCESS location Z, NOACCESS height after partial location read, stale and
released pages; unchanged unpublished outputs; null/alignment/overflow gates;
non-memory exception filter unchanged. Runtime outcome remains pending.

Observer correction: require full-process visibility (sandbox hid the live
Steam game), include actual `perf:`/`xr-heartbeat:` log names, retain a passive
post-exit collector after the active 15-minute observation bound. Earlier
observer stopped at 14:14 with zero selected events; crash occurred at 14:36.
We recovered the authoritative native crash log/dump, but do not claim live
observation covered that late crash. Next test starts only in a current test
window; user stays in headset, normal closure and verified DLL guard remain.

Next candidate L2_PASS: source `f69caf3e8160f836ed0f2fc3282f88fad74edad1`,
normal x86 PE32 /O2 build `4483CF02D86D6EB466CB9A2A924B4401329BD0345544BD5BA2254814E305A921`.
Installed DLL-only; deploy backup `deployment/install-backups/20261013-145004`
contains verified D8928180 baseline. INI/UI unchanged. Restore guard armed
with 600-second launch window; wait for user readiness rather than consuming
the physical test window before they put on PSVR2. L3/L4 still pending.

## PID4672 result and next controlled candidate

4483CF02 session: user confirms New Game -> intro without crash; native log
explicitly records the guarded read rejecting old pawn0x0DAAA000, then a new
pawn is acquired. This is L3/L4 acceptance of this crash reproduction only,
not every UObject transition. Helicopter still shows different stretched eye
halves. CINE turns ON and stays ON; no successful composition log. Several
later native Present windows ~87-90 FPS, not a complete user FPS measurement.
User confirms closure; guard restored D8928180 with INI/UI hashes unchanged.

Initial lead was the RT->offscreen plain StretchRect restriction documented at
https://learn.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3ddevice9-stretchrect.
Native x86 D3D9Ex GPU test FALSIFIED surface type alone: this driver accepts
both original copies (all HRESULT S_OK). Do not replace that path on this
hypothesis. Next candidate retains composition behavior and adds max20
gate/stage/HRESULT log records from existing calls, no new draw queries or
readback. Falsifier: armed+copy stages succeed and image still wrong -> inspect
capture stamp/bridge pixels, not further surface guesses.

User-requested intro change: halve both width and height relative to accepted
per-eye Bink placement, retain centre/UV/aspect. `Render.VideoCinemaScalePct`
defaults50; 100 is exact legacy control. Installed INI stays untouched; no
world/menu/glove/weapon/shadow/input transform changes.

Dubbing audit: PolishLanguage=1, localization hooks initialize, installed story
manifest plus992 WAVs exist. Existing PolishMaybeSpeak is driven by Localize
text callbacks and PlaySoundW; it does not replace a Bink movie audio track.
This is a movie-audio integration gap, not evidence the translation pack was
forgotten. No voice assets changed; old deployment-status prose is historical.

Candidate prepared: source4ba217495cee6d82e9a7ae574050786597836866, normal x86
PE32/O2 SHA3242DB3167CF0A33476CD565EA5D6A6D70303DD60E532B1D76900C14D98CED4D.
Placement/mono/scene/pawn regression and runtime-policy tests PASS. DLL-only
deploy backup20261013-152140, verified restore guard READY, 600s launch window;
INI C59B1CF6 and UI5B2EA469 unchanged. No helicopter/Matinee fix claimed from this
build. Next route: New Game, assess smaller movie, remain in helicopter ~20s
for new gate/stage logs, close normally. No mid-session chat or unchanged
baseline visual question; full session evidence collected on exit.

## PID40412: accepted Bink size, proven first copy failure

User: no crash; films now excellent. Helicopter: different stretched eye
images and apparently semi-transparent NPC face; no Polish helicopter voice.
Native log confirms video50% placement. CINE enters CutsceneFlat; composer
armed, then `copy-source-to-temp hr=0x8876086C` (D3DERR_INVALIDCALL), no success.
Earlier small GPU test did not predict this runtime failure: keep both facts.
The current zero-copy capture successfully copies the same backbuffer into a
DEFAULT-pool render-target texture. Hypothesis: use that demonstrated target
kind for the cinematic scratch instead of offscreen plain. Do not modify NPC
shaders from this image symptom. Falsifier: copy still fails, or both copies
succeed but submitted eyes/content remain wrong -> inspect the actual next stage.

Small controlled change: one scratch RT texture/surface pair at source size
and format, cached and released on Reset; scoped viewport/RT/depth/scissor
restore and pixel rollback on failure. Share helper with offline GPU test:
full5264x2684 synthetic quadrant RGB/alpha, whole-source fit into left half,
bars, state restoration, injected failure without damaged source. No game
assets in test. Keep accepted video size, pawn guard, shaders/input/menu intact.
Native log and observation saved; baseline D892, INI C59B1CF6, UI5B2EA469
restored/rehashed. Helicopter dialogue is a separate UE3 audio gap, not Bink.

RT candidate L1/L2_PASS: source468c74f328fcc4e85794c035097b74d613fe0d63,
normal x86 PE32 /O2 DLL983BED0ADDFBEF64927800F0F25481D7DF35BF2E7E37E1A8BD7E1CC8E5B9F7F1.
Native synthetic shared-helper test PASS at both sizes: complete source
quadrants/opaque alpha, bars, pixel rollback after an injected post-clear
failure, exact viewport/scissor/target restoration, auxiliary MRT identity
and untouched MRT sentinel pixels. Placement/mono/scene/pawn and accepted
runtime-policy regressions PASS. No game pixel or physical PASS inferred.
Installed DLL-only with backup20261013-160811; guard45624 READY with600s launch
window, original INI/UI hashes unchanged. Next physical check: whole matching
eye image/proportions, NPC face, head direction, hands/FPS and clean return.
Do not switch to chat during the headset run. Dubbing unchanged in this build.

## PID44632: successful full image, next framing/foreground gate

983BED0A session: composition S_OK and bridge confirms identical full source
for both eyes. User says image is good overall; still too wide/stretched and
edges outside comfortable view; tracked glove seen moving at edge. Native
log verifies no copy errors and ~89-90 FPS late in this cutscene. Guard restored
D892 and MAIN rehashed DLL/INI/UI. This is acceptance of full-image mono copy,
not final aspect/framing/foreground acceptance.

Source mechanism: ApplyVrFov still derives HALF-width tanX when g_dupDraws ON;
SetVSConst forces it even though StereoPairBody's CutsceneFlat gate executes
one original draw over FULL width. Pure projection test proves same geometry
has 2x horizontal pixel scale. Controlled correction uses full-width tanX only
for confirmed CutsceneFlat; XR submission keeps eye tanX, all other modes
unchanged. Falsifier: runtime full-viewport diagnostic executes but the face
still stretches -> inspect measured incoming/outgoing projection and source
pixels, not arbitrary width squashing. Fit85% reduces both dimensions equally,
center and complete source preserved, Bink50% untouched.

Hide only already classified tracked foreground in CutsceneFlat, after mesh
learning, with scene-target/non-shadow/FgMatchWindow + exact hand section or
existing gun/ride identity. No world/NPC/shadow/animation/input state changes.
One success log verifies whether the gate actually suppressed the reported
glove; no log means next test must not claim that fix executed.

Audio audit: Steam appmanifest language is english (two entries), independent
of mod PolishLanguage1. Intro file already has Polish narration audio ordinal
10 / actual track ID0xa, matching SIDE's offline inventory. Native import and
installed binkw32 exports confirm _BinkSetSoundTrack@8 and _BinkOpen@8; actual
track selection still unobserved. Do NOT infer it from the language flag, or
force track10 for all movies (others have9/10/11 tracks and content unclassified).
Reference for track-ID array semantics: https://www.radgametools.com/bnkhist.htm.

Helicopter dialogue: runtime code only calls WavForEnglish from rewritten
Localize FString; 40-character gate and exact text lookup. The package/asset
audio lookup exists but has zero runtime caller outside its definitions; no
SoundNodeWave playback redirect. Prior session logs4 Polish voice calls before
CutsceneFlat entry, zero during it (old budget8 not exhausted). These facts
explain why installing992 WAVs doesn't establish cutscene dubbing. Next build
observes Bink exports max12 calls each, forwarding original arguments/results
unchanged, plus max16 cinematic Localize strings/mapping and16 PlaySound
acceptance notes. SEH-guarded argument snapshots; no BINK struct offsets,
audio mixing/track override/asset/config mutation. Native audio entry missing
mapping versus unplayed mapped voice remains to be separated by runtime data.

Framing/audio observation candidate L2_PASS: source22a68c74e47e9832997467a0b1589a359ddbf6af,
x86 PE32 /O2 SHA820E82639FA89FD13B6DFF7B0E6F15B15DF06FD0D66362123AFCE3A88A5F98FC.
Six narrow executables and runtime-policy PASS. All25 installed movies match
SIDE's known source inventory hashes; no file copied or replaced. DLL-only
deploy backup20261013-164928; guard41384 and read-only observer42416 READY,
600s launch window. Installed INI C59B1CF6/UI5B2EA469 unchanged. Test pending:
New Game -> movie -> helicopter20s, no chat switching, normal close; report
framing/proportions/foreground and heard languages. No audio fix claimed.

## PID5052: new DLL confirmed; actual handoff and audio failures

Session16:58:05 includes the new Bink observation hooks, so it ran820E8263,
not an earlier DLL. On normal exit the guard restored D892 (independently
rehashed); INI C59B1CF6/UI5B2EA469 unchanged. Native SetCinematicMode ON arrives
during intro, but camera1A3B9300->2D426900 during VideoFlat/loading clears CINE
because the old pawn/menu identity has not survived. Consequently there is no
composer/projection/foreground success; rendering stays stereo. Do not treat
this run as an executed test of the85% fit or foreground hiding.

Controlled hypothesis: an observed video-to-world handoff while the native
cinematic ON signal is still present must not be treated as a cutscene EXIT.
Add only this narrow exception to the existing camera predicate (VR + flat
policy, VideoFlat, raw cinematic1), keep cache/pose reset and other camera/menu
clears. Explicit native OFF and subsequent non-video level/menu transitions
remain authoritative. Falsifier: exception logs but no composition, or cutscene
does not end at gameplay -> inspect that next signal rather than holding CINE
globally. Full helicopter-end/gameplay/pause return is part of the next gate.

Bink native selection: count4 IDs0,1,2,3 before opening the named intro, so
known Polish track10 is not loaded. Only for that exact intro basename and
known0..3 selection, replace English narration1 with10, preserving0/2/3.
Configure immediately before native Open, preserve its default0 reset on
success and restore prior selection on failure; serialize this global setting
among the existing exported API hooks. No other movie override/voice-file changes.
Pure shared policy/forwarding tests must establish exact call order and fallback.
Helicopter dialogue remains separate: native SoundNodeWave audio is not wired
to the installed package/asset WAV lookup. No dubbing claim for that path yet.

Installed binkw32 disassembly proves the global config mechanism: exported
SetSoundTrack RVA0xD320 copies count/IDs into globals; Open RVA0xD690 reads that
selection into the opened handle (RVA0xE0D7..0xE106), then resets active global
selection to count1/ID0 (RVA0xE116..0xE123). Addresses are offline evidence only,
NOT runtime offsets. Production resolves exports and uses API calls, no BINK
struct writes/global memory patches. Preserve native default post-Open rather
than restoring count4 on success, which would change native lifecycle.

Controls: Render.CutsceneVideoHandoffFix and Localization.PolishIntroAudioFix
default1 in this controlled branch; existing flat-mode/Polish-language switches
still gate behavior. Installed INI untouched. No fixed-track rule for
unclassified films, no subtitle-driven claim for all992 voices. Bink basename,
actual native flags/layout, Polish OFF, binary buffers, invalid memory, forwarding,
successful default reset and failure rollback tests PASS. Movie->world/native
OFF->gameplay->pause policy, original camera clears and GPU85% pixel/state/MRT
rollback regressions PASS; actual scene exit and audio listening still pending.

Candidate ready: sourceed9b1bfacd58e520e8ad34d97b1c20f14ac68db5, regular x86
PE32/O2 SHA8B34C584B4FD9AF1774A8B2FDEDC4646746D33E1F055B68CA54E62C7E3070384.
Intro AF53C8F8...CA9A3686 independently rehashed; installed DLL exact match.
DLL-only backup20261013-173236 verified; guard7664/observer42544 READY (600s
launch window). INI C59B1CF6/UI5B2EA469 unchanged. Bink/movie/WAV files untouched.
Pending physical script without chat switching: New Game, listen intro10-15s
(may then skip), let whole helicopter scene finish, verify restored gameplay
stereo/tracked hands plus pause/Options/resume, close normally. Observe aspect,
edges/foreground and audio; logs must separately prove handoff, composition,
foreground suppression, intro track selection and exit. Helicopter Polish
dialogue remains unimplemented, not included in the intro voice acceptance.
