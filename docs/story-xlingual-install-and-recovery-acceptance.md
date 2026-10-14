# Story xlingual installation and recovery acceptance — 2026-10-14

## Physical result / cleanup

Steam PID35568, verified EA9EFFB1 runtime source c51f53a. User: everything OK
except Polish dubbing, note not checked. Clarification: Continue/load at the
first building, NPC and radio spoke English. Do not infer helicopter, intro,
new-game or note acceptance from this trial. Existing log proves XR submission
and true stereo, frequently90FPS, also some dips. No blanket performance claim.

All three guards completed. Independent core/asset/WAV identities returned to
their original baselines, explaining the subsequent TV reversion. On the user's
new request, persist the reported-good runtime pair plus its INI, not another
temporary launch. Installed:

- Renderer EA9EFFB10C83815DAA36131F1F7785D2CC1B83C55866DF84AD0700CC335FD71C.
- Injector F5137234C9BACCCF636A6A387B63CCD9865C726C9E662AD56B6E30C74770F07A.
- INI757F414B1D4F38D26E3F234B75D05EE14FB78BFB702107BF555C70612C4BF0FE.

Private runtime backup: deployment/install-backups/20261014-205618.
The explicit RuntimeDllPairOnly+IncludeIni path owns exactly3 backed-up files,
no textures, game packages, UI, banks or intro. Its fake install/rollback test
passes. This is user-directed persistence, not an unattended experimental lease.

## Requested new story pack

Source: local SingularityVR-cutscene/audio/polish/story_voicepack_xlingual.
Exactly992 WAVs and manifest entries; same992 installed names, all SP/sp packages,
mono PCM16/24kHz. No regeneration. The two R6 files are included;825b's middle
phrase remains auditory QA pending. Source existence/format/hash is NOT ASR or
acting acceptance. `install-story-xlingual.ps1` narrowly backs up and replaces
only992 story WAVs; hashes894 protected UI voices before/after. All sources and
backups verified. Unknown names/layouts, running game and reparse files refuse.
Rollback verifies every backup before writes and retains unexpected user edits.
Private receipt: deployment/hands-dubbing/story-xlingual-installed-20261014.
Normal install is persistent as requested; no game automatically launched.

Legacy installer's story source points at xlingual now, but MAIN deliberately
does NOT run that broad installer: it also writes UI/VFX/config and sets
PolishLanguage=0. Its one-line source change alone is not a safe deployment.

## PR status / remaining actual playback path

GitHub PR21 (`agent/polish-xlingual-v2`, head0aa7585) merged as75db8bc at
16:00:26Z. It contains the43 audio text/tool/QA files, no WAV distribution.
PR20 (`agent/polish-xlingual-dubbing-20261014`, head1532907) is still a conflicting
draft containing inherited renderer changes too; do not merge it as audio-only.
No duplicate SIDE PR or MAIN merge performed.

The runtime's PolishMaybeSpeak follows localized text lookups; native SoundNodeWave
playback is separate. WavForEnglish skips texts marked native_replaced when
NativePolishSpeech is ON. A copied992-WAV library does not re-encode native PC
Vorbis bank payloads or prove that NPC/radio SoundCues consume those files.
The recent log has no `polish voice:` playback line. This is a missing witness,
not proof that all sounds were absent. User explicitly heard English.

Next justified step: identify the exact loaded first-building radio/NPC wave
and native package, compare live identity/content to the pinned native-bank
inventory, then integrate matching NEW xlingual PCM into that bank using the
existing exact-sample/loudness/independent-roundtrip pipeline. Do not merely
ask the user to re-test the same English baseline or globally replay/mute audio.
Preserve intro,UI894,multiplayer and accepted renderer. Font/translated note
data were restored by the earlier asset guard and need an explicit next lease;
the unchecked note is not silently promoted as verified.
