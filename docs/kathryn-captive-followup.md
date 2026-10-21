# Kathryn radio and captive scene — unresolved physical result

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

## Scene experiment, not a speculative cinema fix

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
