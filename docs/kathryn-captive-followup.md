# Kathryn radio and captive scene — unresolved physical result

## Latest physical receipt: CD571D46, 2026-10-21

User clarified: native hands visible DURING cinema; free tracked hands during
the subsequent escape are the missing restraint. Kathryn radio remains English
and scene presentation remains unsatisfactory. None is physically accepted.
The outer event bridge had zero logged invocations although its names resolved
early. Existing SetCinematicMode ON/OFF and mono composition did execute; logs
do not identify those windows as the exact failing Kathryn sequence. Repeating
the bridge unchanged or forcing cinema after every cinematic exit is unjustified.

Preserved private log: build/combined-followups/kathryn-fix-5832409/physical-20261021-1201.log,
SHA256 BCCB78DA5C82722A75568045F6BDDF4FFEAB39573B95ED9586CF0B95B2F4B9CC.
Guard successfully restored26 native banks first, then core, full accepted
postflight PASS. MAIN independently rehashed accepted DLL EF3A1F3B / INI55A3ED4D;
neither Singularity nor the guard remains running. Older "No Steam writes yet"
paragraphs below describe preparation only and are superseded by this receipt.

Offline RE confirms EPlayerHandsStatus has only NothingEquipped/TMD/Weapon/MAX,
not a bound state. The shared RvGame asset package has original noTMD/TMD arm
meshes but no literal restraint asset identity; this does not exclude level-local
animations. The old two-Outer witness stopped at TheWorld, NOT the map Package.
Expanded default-OFF scene witness now walks a guarded/cycle-checked maximum16
Outer chain to a verified Package class, and records pawn, named native hand
components, SkeletalMesh and Animations-root identities. Offset discovery uses
the existing validated reflection layout, cached per class. No new hooks, GPU
reads, state classification, engine writes or automatic restraint timer. Root
identity is NOT an active-animation traversal or proof that rope pixels exist.
Hypothesis: escape retains native animation ownership independently of cinema;
falsifier: actual native components/animation producer show no bound animation
when free controller hands are observed. Need exact producer before policy edits.

Audio coverage is also partial, not a failed copy: exact package/asset audit of
the staged lookup rows labelled speaker_guess=kathryn finds263 rows:95 explicit
native replacements,167 retained,1 not covered. These counts differ from reports'
cast/asset-based "97 Kathryn" total; do not combine different selection rules.
Examples RT1_2008_KAT and RT1_2005_KAT need tempo2.29/2.20 to fit original windows,
so accepted audio was intentionally retained. No cropping/unsafe speed-up allowed.
New tools/audio/audit_native_voice_coverage.py refuses overlapping receipts and
never interprets lookup native_replaced=1 or a staged WAV as delivery proof.
It does not identify the line actually heard or validate live file hashes.

## Observed versus inferred

Physical feedback: bound hands absent; Kathryn radio English; Kathryn scene stays
in stereo and character flickers. Other requested tests were not performed.
Active candidate F2387D10 and flags are confirmed by the saved runtime log.
SetCinematicMode ON/OFF events exist, but the log does not tie them to this scene.
Do not interpret a function name, foreground count, or PlayerWalking as content
proof; do not infer a level number from checklist item4.

Private log: build/combined-followups/native-axis-5370fb8/physical-20261021-1051.log,
SHA256 7F012A59FBA63A0300803381B1CA07156132DE1FF121CF3065F4645AFAE40D4B.

## Proven native audio delivery mistake

NPC/radio consume native SoundNodeWave Vorbis, not the overlay WAV directory.
Localize/PolishMaybeSpeak does not supply these native voices; the session contains
no `polish voice:` playback. Lookup native_replaced rows deliberately suppress
overlay speech. Merely changing their WAVs cannot deliver the new performances.
See native-npc-radio-producer.md and SIDE-AUDIO-INVESTIGATION-20261014.md.

prepare_xtts_native_banks.py reads exact accepted banks and staged WAV hashes.
It preserves native properties/cues, duration/frame counts and unrelated exports/
platform audio; overlong takes retain accepted audio rather than being cropped.
The fit ceiling is1.35, below the helper's absolute1.85 bound. Independent
unpack_chunks decoding must equal the expected logical package, not just our own
container inverse. Output cannot be inside CookedPC; tool never deploys.

Local reports under build/combined-followups/xtts-native-kathryn-20261021 and
xtts-native-kathryn-remaining-20261021 cover29 pinned banks.26 have candidates:
144 replacements including97 Kathryn takes;228 retain accepted audio. No-safe
banks: SP_VI6_kismet02_LOC_INT, SP_VI6_kismet04_LOC_INT,
SP_Freight_S_StreamDock_LOC_INT. SP_VI6_kismet03_LOC_INT was excluded because
the accepted profile does not pin it. These are OFFLINE_CONTAINER_PASS only;
actual heard radio identity and auditory quality remain unverified. Large private
game packages/audio must not be committed.

## Controlled correction: outer script-event coverage

Owned RvGame.u export11870 contains a49-byte script body delegating all six
named bool parameters from SetPlayerCinematicMode to SetCinematicMode. This is
a native-to-script event, NOT a native exec wrapper. ProcessInternal need not
re-enter the hook for the inner script-to-script call. OnToggleCinematicModeNative
instead consumes one Action object; it must never be decoded as bools.

Render/CinematicEventBridge=1 covers that outer event through the existing hook.
Default OFF; exact Function/owner class, local Camera.PCOwner identity, six named
BoolProperty offsets0,4,8,12,16,20, readable voted FFrame locals and canonical
0/1 words are all required. Any mismatch refuses without touching engine data.
The resulting mod cinema bit feeds the existing presentation/native-hand policies;
there is no forced scene name, timer, new native hook or engine parameter write.
Forty CPU gate checks and five hand/bridge source guards pass. Runtime match,
bound-hand pixels and Kathryn cinema remain UNVERIFIED. Falsifier: no accepted
outer event in the scene, or the existing cinema bit already ON when it fails.
In either case investigate the actual camera/mesh producer, not another override.

The next candidate contains this correction AND the read-only witness below,
not a diagnostics-only session. Native audio uses26 bank candidates, not merely
overlay WAV copies. Three no-safe banks and228 retained takes remain unchanged;
do not promise every English line has been replaced.

## Bounded scene witness accompanying the correction

Owned RvGame exports prove RvPlayerController.SetPlayerCinematicMode and
OnToggleCinematicModeNative exist. Their invocation in the failing sequence is
not yet proven. Diagnostic/CinematicSceneProbe=1 watches them via the existing
ProcessEvent hook (24notes maximum, exact recipient-class gate); no parameter
reads or inferred mode writes. The existing validated StateFrame observer records
controller/pawn states, camera, two Outer ancestors and indexed foreground
seen/kept counters. One-Hz sampling, transitions/10s heartbeat, maximum120notes.
Default OFF. No new native hook, GObjects scan or GPU readback.

Hypothesis: this sequence bypasses the currently observed cinema signal, so both
presentation and native hand ownership retain gameplay policies. Falsifier:
actual cinema flag is already ON during the failing sequence. In that case
investigate camera arbitration and the exact hand mesh producer instead of
inventing another timer/state-name heuristic. Counters alone cannot identify rope
or hands, and this diagnostic does not fix their visibility.

Next physical route only after MAIN finishes exact deployment/backup preflight:
load the same save, watch captive/Kathryn scene without switching to chat, close
normally and report. Collect producer log automatically; never request typing in
headset. No launch is part of this offline preparation. Stable accepted runtime
and assets stay installed until a fresh start.

## Prepared diagnostic receipt

Superseded for the next round by correction source5832409aae13b010f38b6b0835e1569616232f8c:
x86 PE32 `/O2`, no PERFORMANCE define, tracked source clean at build (only private
untracked deployment/leases). DLL
CD571D464085D3C77696DF40A7B4BFE40EFE744692AF032021E836B27EE2C055;
INI7BE40621888DB8913C1F6BA1388CDB9027DC984EDA5708AA9979284E92095673.
Private core artifact build/combined-followups/kathryn-fix-5832409 adds only
Render/CinematicEventBridge=1 to the prior diagnostic candidate's INI.
Official core `Install -WhatIf` and native-bank `Status` preflights PASS, read-only.
Native manifest build/next-weapon-reload/xtts-kathryn-native-20261021/asset-manifest.json
SHA F3E705731A7ED09409413806BAE12373135D21C9C2B30481DF737472AA8FCE9C.
Python tests: hand/source5, native-bank4, staging3, session-assets6 PASS; CPU bridge40
PASS. The earlier mistyped test_prepare_xtts_native_banks.py path did not run;
the actual test_xtts_native_banks.py was then run and passed all4 tests.

Fresh-start installation sequence: confirm full-process visibility and game exit;
deploy core with its original exact plan FIRST, then26 native banks with their
separate asset manifest/backups. Arm a combined exact-hash lease before launch.
Restore banks BEFORE core rollback, so the latter's full frozen profile validates.
Do not reuse the nine-reload-asset guard or the overlay-WAV guard: neither owns
this26-bank transaction. Preparation is not a claim that such a lease was armed,
nor authorization to launch without the user's fresh start. No Steam writes yet.

### Earlier diagnostics-only artifact (not the next candidate)

Source f39008fd88ae8121c7e0b8085dffaec33f7e3b61; x86 PE32 `/O2` PASS.
DLL D7DA8A8C1179F336437EDD957E01ABC9B306F2EFA4A526F113414FAC5D17910E.
Private artifact: build/combined-followups/kathryn-scene-f39008f. Tracked source
clean at build (only private untracked deployment/leases). Native-hand guards4,
native identity tests4 and room voice audit5 PASS; opening-dialogue fixture test
not run (requires its separate original intro fixture). Read-only official
accepted-profile install preflight `-WhatIf` PASS. No installed files changed.
Candidate retains prior five test flags and adds only CinematicSceneProbe=1;
native-bank candidates remain separate, not advertised as installed or accepted.

## Restore evidence

DLL EF3A1F3B9F0641A291F9285DD7283EC7D3B4433A8F87BC210DBC98B721B7B753;
INI55A3ED4DBC6E158F01A2219177D825C066E7306DABCE6ACFB1C7118EC9E3F659;
lookup579C82AE3C97DA76A5829E8E019FBEBBDD0454C99E0435C4B592E5159D8AF5BF.
Full audio audit:1667 accepted matches,0 candidate/unknown matches,0 added files
remaining. Guard hit ERROR_ALREADY_EXISTS on Move-Item; Copy-Verified now uses
same-directory File.Replace for existing files, File.Move only for new files.
Synthetic test covers replacement, creation, wrong hash and running-game refusal.
This is not a claim of testing the repaired restore path against Steam.
