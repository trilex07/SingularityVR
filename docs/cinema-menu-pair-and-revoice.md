# PID34980 physical result and bounded follow-up

User: no FPS dips noticed; no trigger regression reported. Helicopter Polish
speech improved but remains much too quiet; gameplay English remains. Hands
are still too far from the face (approximately50cm apparent distance when
controllers are near the face), not accepted. Pause during cinema is small and
double; returned main menu is single but background differs between eyes.
Thus frontend handoff solved menu duplication only, not all frontend stereo.
Four originals and frozen files independently restored/hash-verified.

## Presentation hypothesis / falsifier

Actual class3 pause submits cinematic crops while the composer consumes the
full SBS backbuffer. It scales already-separated UI with the cinema image.
Class2 after exit now retains two half-rects, so background differences remain
even though UI is no longer doubled. CutsceneMenuPairFix defaults OFF: identified
focused class3 + native cine + paused takes MenuFlat ownership; focused class2
and this cinema-pause class3 stamp the existing identical-crop bridge. No asset
layout edits, no native cinematic flag override, no gameplay pause policy change.
Cinema pause retains native camera isolation. Resuming re-enters actual cinema;
native OFF restores stereo normally. A confirmed Bink without actual pause keeps
its priority. Falsifiers: class3 native pause predicate never fires; duplicate UI
or wrong background remains despite same source crop; resume changes camera or
video behavior. No physical acceptance from unit tests.

HandAnchorProbe defaults OFF, eight fixed records at least2s apart, existing
exact noTMD section/non-shadow gates. Copies original/posed wrist constants,
local230, existing controller matrices, desired hand-head offsets and gun anchor;
flushes outside native Draw in Present on the same thread. No added hook, asset
read, COM resource, skin mutation or pose correction. This distinguishes shared
gun-pivot residual offset from input scale/camera origin errors before guessing
a50cm translation. Raw palette translation is not asserted as a world joint.

## Requested1886-voice candidate

Source a390ede / feature/polish-dubbing-full-qa, user's explicitly named private
full_revoice_intro directory. Manifest894UI+992story, QA1740automated screen
PASS/146review. New hashed filenames differ from installed lookup filenames:
installer validates exact identity then maps previous_audio basename to installed
manifest reference. WAV-only scope: no VFX/DLL/INI/text/voice-manifest/Bink changes.
All source monoPCM16/24k RIFF chunks, exact source/target hashes, verified backups
before writes. WhatIf and corrupt-backup/user-conflict rollback tests use fake
installation; pendingQA requires explicit flag, not auditory acceptance.
Temporary Steam candidate must have its own no-launch/after-exit restore guard.

IMPORTANT: existing PolishMaybeSpeak uses Localize text and PlaySoundW, does not
redirect/mute native UE3 SoundNodeWave gameplay. Installing the requested pack
does not claim992native gameplay voices fixed. This pack contains no native
INTRO_1005..1016 identities; it does not replace the ten helicopter waves.

Helicopter gain follow-up: explicit +6LU target fails bounded convergence, +3LU
fails encoded true-peak headroom. Both private attempts retained and rejected;
do not relax quality gates or quietly ship clipping. Native mixer/speech balance
remains open. Original loudness-matched package retained in the menu experiment.

L1_PASS: scene ownership/resume/video sequences; fixed record budget, finite
inputs, tick-wrap/cadence and pending isolation; existing glove/palette tests;
camera lookup/pawn stale-page safety, Bink selection, hotpath and runtime policy.
L2_PASS: regular x86PE32/O2 source7cee8ba, VR21515F87. Offline four-file staging
adds only NoTmdPaletteFix/CutsceneMenuHandoffFix/CutsceneMenuPairFix/HandAnchorProbe;
fake Steam lease/WhatIf/path/corrupt backup/conflict/restore tests PASS.
Full1886WAV fake install/restore and fresh WindowsPS hidden60s timeout/restore
PASS (no Steam launches). No L3 or physical acceptance for these new policies.
