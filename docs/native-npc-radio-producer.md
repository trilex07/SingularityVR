# Native NPC/radio producer at the first building — 2026-10-14

## Observed failure

Steam PID35568 (DLL EA9EFFB1, source c51f53a): user loaded a save at the
first-building entrance; NPC and radio spoke English despite 992 xlingual
WAVs installed under `Binaries\vr-assets\audio\polish\story`. Note unchecked;
intro/helicopter/menu not in scope for this trial.

## Hypothesis (single mechanism)

NPC/radio dialogue never passes through the mod's Localize text hook, so
`PolishMaybeSpeak` can never fire for it. The only producer is the native
UE3 `SoundNodeWave` payload inside the CookedPC `SP_VI2_*_LOC_INT.xxx` banks.
Copying WAVs into `vr-assets` cannot change that audio.

## Supporting evidence

- `session.log` (recovery-live-20261014-2035): **0** `polish voice:` lines.
- `polish localize saw:` lines contain ONLY menu/UI strings (`Start Game`,
  `Main Menu`, `Continue Game`, `New Game`, `Load Game`, ...). Zero gameplay
  dialogue lines reached `PolishRewriteFString` during the whole session.
- Code path (`spikes/view_matrix/d3d9.cpp:21403`): `PolishMaybeSpeak` is
  called exclusively from `PolishRewriteFString`, and only for `chars >= 40`.
  Short barks (`Shoppers down!`, `Help! Help!`, `Let's reach the snowball!`)
  are structurally excluded even if their text ever arrived.
- Installed `polish_story_voice.jsonl`: 992 entries, **0** with
  `native_replaced=1`, so `WavForEnglish` would have served a file — if any
  NPC/radio text had been looked up. It was not.
- xlingual manifest maps the dialogue to native assets: 40 rows for
  `SP_VI2_kismet01/02/03/04_LOC_INT`, e.g. `VI2_5020_DEMY` (wounded NPC:
  "My leg is hurt..."), `VI1_1005_GST` / `VI1_0105_CP2` / `VI1_0113_CP2`
  (tower radio). All 40 source WAVs present, mono 16-bit, 1.6–10.2 s.
- Native originals untouched and hash-pinned: kismet01 `CFCFBD65…`,
  kismet02 `44C42E06…`, kismet04 `84773631…` match `prepare_native_story.py`
  / `prepare_room_banks.py` pins. No bank integration has happened yet.

## Falsifier

If any NPC/radio line appears in a future `polish localize saw:` / `polish
voice:` log line, the text path is live for that line and the hypothesis is
wrong for it — then a manifest/text fix suffices and no bank edit is needed
for that line. Absence confirms the native-bank route.

## Smallest next experiment (offline first, then one headset run)

1. OFFLINE (this checkout): decode the 4 VI2 banks read-only, list
   `SoundNodeWave` exports + `SpokenText`, and join against the xlingual
   manifest on `(package, asset)`. Requires uepkg-compatible decoder +
   ffmpeg; neither is on PATH in this checkout — locate before editing.
   No Steam writes.
2. RUNTIME (needs user `start`): bounded native `SoundNodeWave` load witness
   (default-OFF, counters only, no audio change) during load-entrance-save
   → tower-radio sequence, proving the exact loaded wave/package identity.
3. Only then: integrate the matching NEW xlingual WAV into that bank via
   `native_story_package.build` (exact frames/loudness/LZO roundtrip),
   preserving cues/metadata/duration. Never generate dubbing from scratch,
   never globally mute English, never blind-PlaySound.

## Freeze for the next test (done, offline)

## TV two-image root cause (proven 2026-10-14, probe CF96A841)

`mirror-diag:` windows from PID43880: every window `n=600 skipRect=600`,
`ok=0`, `bb=0x0`. `skipRect` = `!dev || presentSrc || presentDst`; dev is
non-null (game runs), so the engine passes explicit Present rects on every
frame and `desktop_mirror::Apply` returns before touching the backbuffer.
The code comment already named the gap ("Subrectangle Present needs a
separate mapping contract") — the contract was never implemented, in the
accepted build either. Falsifier: any future window with `ok>0` proves rects
are sometimes null. Next justified probe: bounded one-shot log of the actual
`presentSrc`/`presentDst` values (coords + which of src/dst is set), then a
subrectangle-aware mapping. No pixel-path change until rects are known.

`deployment/hands-dubbing/current-install-freeze-20261014/freeze.json`:
3 runtime hashes (EA9EFFB1/F5137234/757F414B), 992 installed story WAV
hashes (all match the install receipt candidates), 8 VI2 native-bank
original hashes. Next test restores THIS state — not `recovery-ready`
(A8F38EF6-era), not `quiet-revoice-v2`. `tools/run-recovery-trial.ps1`
still points at the stale manifests + `install-revoice-candidate`; do not
run it without new manifests.
