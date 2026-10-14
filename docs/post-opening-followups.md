# Post-opening follow-ups (after PR17)

Accepted snapshot PR17 is merged into main8751a27. New work is isolated in
`fix/post-opening-followups`, not a retroactive claim that PR17 fixes all voices,
weapons or dialogs. MAIN owns Steam; preparation below is offline only.

## Evidence and smallest experiments

- **Room recordings:** user tried several English recordings, one mentioning
  research lab. Installed manifest identifies VI1_1511_ANN in two zone02a banks.
  Previous delivery changed only kismet01/intro, not these banks. Native
  SpokenText confirms the earlier PL draft omitted weapons/agriculture/medicine/
  chronology clauses. SIDE17e2f6b supplies13 unique WAV/19 bindings, complete text,
  original property offsets/hashes and bounded timing; NOT_AUDITIONED. MAIN now
  encodes2 exact banks. Full resampled speech must fit before padding-only trim,
  decoded mono48k frame counts stay exact, LUFS within1 of native, peak<-.2dBTP.
  Independent original/control/candidate LZO roundtrips and property/platform/
  cue identity checks PASS. Falsifier: a triggering room announcement remains
  English despite the loaded candidate bank, or dialogue timing is wrong. No
  blanket SoundCue/mixer or accepted intro/radio changes.

- **Notes:** actual log focus SPNotepad; 72 Notes entries in installed manifest,
  most `skip`, including nearby WelcomeCenter/ClosingDown.15 first-area key/hash
  translations (incl ReadNote) are prepared; NOT all72. English is read from the
  user's own manifest and SHA checked; unrelated rows remain byte-identical,
  input tokens and paragraph boundaries retained. Censored letter/other areas
  remain open. Existing Localize guard also rejects >480char notes (actual
  paragraphs reach880); bounded4096 wchar/16KiB UTF8 path uses existing validated
  FString assignment. Lookup now refuses buffer truncation instead of returning
  a partial paragraph. Falsifier: a patched note still stays English; then trace
  its producer rather than guessing another GFx variable path.

- **Lector:** UI English identities come from UI voice manifest, not story text.
  `Localization.UiNarratorEnabled` defaults0 and is live/persistent in DISPLAY
  as UI NARRATOR. OFF stops only an owned PlaySound UI overlay; native UE3
  dialogue is never muted. Speech lock covers shared repeat guard/play state.
  Quiet delivery applies35% PCM amplitude (~-9.1dB) to894 UI WAVs;992 story WAVs
  byte-identical, frames/format/chunks retained. No global device/mixer volume
  change or runtime audio worker. This is candidate WAV attenuation, not a live
  volume slider or implicit quietening of older voicepacks. Inherited146 QA
  reviews stay pending; attenuation does not claim a new ASR/audition PASS.

- **Pistol:** user says forearms/poor grip persist after pickup. Actual INI and
  initial log have ShowControllerArms1, which bypasses the existing bounded
  forearm hide rules. Next candidate explicitly stages ShowControllerArms0;
  shader constants, weapon alignment and scale are unchanged. Exact arms3314 /
  pistol5799 latch is observed after ARPistol pickup. Grip cause is OPEN, not
  fixed by a guessed offset. Existing read-only wrist probe now reserves a
  second Pawn.Weapon epoch (max16 records/session) to survive first pickup;
  no new object walks, per-draw disk logs or skin writes.

- **Stacked NewGame windows:** runtime proves SPSlots -> dialog. Existing draw
  trace used a sparse hash table but flushed only `[0, groupCount)`, losing
  high-index groups and printing zeros; fixed diagnostic traversal now skips
  empty slots across the bounded2048 table. ModalUiProbe defaultsOFF; four
  cold reflected MovieInfo/RenderTexture/RHI identity notes, no rendering change.
  Generic capture logs include RT/texture/token/class. Next trace distinguishes
  shared RTT/native foreground vs a separately overlaid parent before patching
  ownership. Visual layering fix is OPEN; no speculative suppression of class4
  stations/notepad/gameplay dialogs.

## Gates and delivery

L1 PASS: eight optimized x86 fixtures (native/UI ownership, long-text nontruncation,
two weapon epochs, hands-only, wrist math, menu submit, foreground boundary,
cinema/Bink); Python PCM, Notes identity/token/paragraph and sparse-table checks.
Existing runtime policy checks PASS. Optimized PE32 build is required after the
source commit; generated provenance stays private. No L3/L4 acceptance of these
new changes yet.

Asset lease v1 exact32 is unchanged. Explicit v2 is exactly those32 +two named
zone02a banks, not arbitrary extra files. Same preflight/path/hash/backup/conflict
rules and no-launch/post-exit guard. Fake34 installation/WhatIf/restore/corruption/
conflict/absent-file cleanup/path rejection PASS. Four-file core and1886-WAV leases
remain separate. Real installation remains restored; new test requires `start`.

Private preparation: `deployment/hands-dubbing/post-opening/`: room-banks,
notes, quiet-revoice-v2, assets-ready. Earlier quiet-revoice output has stale QA
file paths and is NOT an install candidate; v2 aligns QA paths without changing
their quality classifications. Existing remasters remain log-only/apply0.

## Physical script, only after MAIN confirms installed hashes/guards

Preparation completed: L2_PASS source c029abe84615123dfb704be7dee2da93515c36cd,
x86PE32 `/O2`, DLL1454A62B0AC28EC9A9C8D3A83E16DD9CF142E6CAEE3220FC591079F015192751.
Core staging `core-ready/test-manifest.json`, exact34 `assets-ready/asset-manifest.json`,
1886 WAV `quiet-revoice-v2` (Plan verified,146 inherited pending reviews). Core4
preflight/fake lease PASS. Fresh hidden Windows PowerShell v2 guard timeout60s
on FAKE installation restored all34, independently rehashed; stderr empty.
Real installed core4+assets34+WAV1886 originals and107 frozen DDS independently
verified unchanged in `installed-baseline-verification.json`. No Steam writes.
Draft PR19 targets main; NOT merged/not L3/L4. Runtime/presentation algorithms
still from the source commit above; this addendum is evidence-only.

User stays in headset throughout, no chat switches/restarts:
New Game -> pause on save-slot/confirmation each~2seconds -> opening -> first room
recordings/notes -> first pistol pickup, hold/move both controllers~15seconds ->
pause/VR Settings DISPLAY, UI NARRATOR OFF/ON (observe no story mute) -> main menu
-> normal exit. Report room Polish/loudness, note text, forearms/grip, UI voice and
dialog readability. MAIN reads exact loaded modules/OpenXR/current logs and
restores after exit. UI changes may cause an INI hash conflict: preserve user
changes and report the actual hash, never silently overwrite or claim exact
rollback. No launch merely to collect unchanged baseline.
