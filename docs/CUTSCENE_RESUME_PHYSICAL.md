# MAIN resume: round 3, no mid-session chat

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
