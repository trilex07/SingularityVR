# Opening native Polish dialogue candidate

Scope: helicopter INTRO speech in `SP_VI_intro_Kismet_LOC_INT.xxx`, not all
game languages or the entire voice library. Polish intro Bink already accepted
and left unchanged. No generated/game audio is committed or redistributed.

## Mechanism and evidence

The inherited generator selected only RL/VI/WL/FR/RT-prefixed assets, excluding
the INTRO-prefixed helicopter lines. The 992-file story manifest therefore does
not include these waves. Existing DLL voice playback follows localization text
lookups, not the native SoundNodeWave playback event; nearby WAVs/PolishLanguage
do not substitute native localized package audio.

Pinned original package SHA256:
`890C61720ABC0BB5AFAD86F16433A811A6F21BA6FC6284563BC5875E52642E85`.
UE3 v584/licensee126, internally LZO-chunked. Independently decoded and checked
with uepkg: 172 names, 5 imports, 143 exports with exact serial tiling; 138 native
SoundNodeWaves, including 11 INTRO waves. Each PC Vorbis stream matches the
previous extracted English Ogg exactly. Tagged SpokenText identifies the actual
helicopter dialogue without ASR guesses. Four declared inline native bulk arrays
per wave: raw/PC/Xbox/PS3, only PC populated; absolute offsets independently
cross-checked against the stream start. No blind integer scanning/offset edits.

Hypothesis: replace the native PC Vorbis payload for these exact assets so UE3
retains its own dialogue event, attenuation, timing and mixing. Falsifier: game
loads another locale/package/wave or continues speaking English after validated
package deployment. This remains a required Steam/physical gate, not a claimed
audio-runtime PASS.

## Candidate and quality limits

Ten speech lines generated using the existing cached Polish Pocket-TTS model,
without model downloads. Synthetic experimental voices use existing Polish
reference profiles (Dmitri/Mykyta/Ruslan); not a claim of original Polish actors
or final approved character casting. Original-English prompts and a user-reference
pilot produced omitted/wrong phrases and were rejected. Candidate completeness
was checked with offline multilingual ASR before/after timing adjustment; ASR
misrecognizes proper names and is not an auditory acceptance test.

Leading/trailing silence removed, complete speech tempo-adjusted (1.0–1.61x),
then padded to exact original PCM sample counts. Mono/48kHz Vorbis preserved.
Every candidate decoded to its original RawPCMDataSize/2 frame budget. Speech
is not hard-cut to fit. Original nonverbal INTRO scream remains unchanged.
128 other waves (including background/AI sounds), all properties, references,
object identities and scheduling metadata preserved. Only declared bulk offsets
and affected export SerialSize/Offset fields relocate. No new runtime hooks or
external PlaySound overlay for these lines; no music/effects muting.

L1_PASS: unchanged repack control; 10 exact replacements; 128 other streams and
original scream preserved; export identity/properties preserved; native offset
and bounds validation; rejection of unknown source, bad logical image, target
set or Vorbis headers. Independent C++ LZO re-decode is byte-identical to the
built logical package; independent uepkg reports zero gaps/overlaps/bad ranges.
Decoder/sample-budget validation PASS. Polish quality in the headset PENDING.

Candidate SHA256:
`C8B30A1B639FBA1A6A5CE58542A81C8687DA4DF5C5BB515EAFC685114B40974C`.
Private artifact under deployment/hands-dubbing/test-artifact.

## MAIN-only test lease

`deploy-singularityvr.ps1 -OpeningCutsceneManifestPath <private manifest>`
delegates to the exact four-file lease: d3d9.dll, dinput8.dll, SingularityVR.ini,
and this one LOC_INT package. All other assets, UI and textures remain frozen.
Candidate INI adds only NoTmdPaletteFix=1; ShowControllerArms and other existing
choices retained byte-for-byte. All four full baseline/candidate hashes pinned,
both DLLs PE32/x86 checked; every original backed up/hash-verified before install.
Never deploy/restore while any Singularity process exists. Real preflight is
read-only. No-launch watcher restores after ten minutes; after game exit it
waits ten seconds for Steam startup/relaunch gaps then restores. It preserves
unexpected user writes, restores remaining owned files, and reports a conflict
instead of falsely claiming exact restoration.

Offline fake-Steam tests PASS in PowerShell7 and Windows PowerShell5.1: WhatIf,
exact install/restore, path escape refusal, corrupted backup refusal before any
write, unexpected INI preservation and remaining owned rollback, frozen UI.
No-launch watcher also tested through a real 60-second fake-install lease:
all four baseline hashes restored automatically, without starting any game.
No real candidate installed/Steam launched during preparation. Fresh user
`start` required. Requested combined trial is intentionally hands+audio+bounded
FPS attribution, not a one-variable FPS performance A/B; speedup attribution
requires a later controlled comparison if needed.

Headset sequence without tab switching: New Game -> accepted Polish intro ->
helicopter/crash -> stereo gameplay for 45–60s, move both hands, L2/R2 ->
Pause/Options/back -> close normally. Observe Polish helicopter speech, glove
shape/tracking and FPS; verify cinema exit/stereo/menus regressions. MAIN uses
existing cinema-exit profiler request only after explicit start and collects logs.
