# Native Polish opening voices — SIDE offline result

Branch `feature/polish-native-timed-voices`, based on MAIN `d59cf1a`.
MAIN exclusively owns Steam installation, package integration and headset tests.
This work does not change renderer/hooks/config or launch the game.

## Evidence and scope

The prepared three-bank native vertical slice had 42 replacement entries.
Comparing each against the exact native `SpokenText` property found 20 text-level
faithful entries and 22 requiring correction (17 unique assets; announcement
banks repeat five affected assets). The semantic review is manual SIDE review,
not an ASR inference or approval of audio acting. The four separately retained
English lines also had mistaken transcriptions/translations; shortening the
previous text alone would preserve those errors.

A separate private candidate contains 29 unique WAVs: four retained-line
corrections, 17 semantic repairs and eight bounded combat barks absent from the
previous 1886-file pack. Existing character references are reused; narrator
references are unchanged for the story repairs. Guard barks use the existing
Ruslan reference, and GST barks the existing Devlin reference, with identity
still requiring human listening. Original death/grunt/breathing sounds remain
unchanged. No claim of all-game native Polish coverage is made.

The input manifest binds package/SoundNodeWave identity, exact native text,
logical-package hash, raw text-property hash/offset, PCM sample/rate budget,
translation notes, previous candidate hash, reference hash and generation
seed/temperature. All original content, translations and generated audio remain
in ignored private output; no original assets or transcripts are shipped in Git.

## Generation and safety

Only the already installed offline Pocket-TTS Polish 6L and cached Whisper-small
were used. No dependency/model installation, network TTS upload, paid service or
full-pack regeneration. Output is mono PCM16/24 kHz. Complete utterances undergo
silence-only trim and a maximum 1.85 tempo change; timing failures are retained as
rejected takes, never cropped into acceptance. All selected 29 WAVs passed
hash/format/waveform/timing checks, with maximum required tempo below 1.821.

Some whole-text takes prematurely ended before their last sentence. Explicit
sentence segmentation with identical word content and independent voice-state
reuse produced the missing clauses. The helper refuses segment omissions and
records the segmented generation provenance. ASR is rerun on the final fitted
audio rather than inferred from the slower source take.

Four synthetic provenance/budget/segment tests and Python syntax checks pass.
This is L1/offline evidence only: no native package loading or auditory approval.
The final ASR screen separately marks pronunciation/content review. In particular,
very short barks and proper names require listening; structural timing success
is not proof that a line was spoken correctly.

## Handoff

Private output: `deployment/hands-dubbing/native-timed-candidate/selected.json`,
`asr-review.json`, `qa.json`; source-text audit:
`deployment/hands-dubbing/semantic-audit.json`.
MAIN must independently verify every source package and selected WAV hash before
integration, and must not automatically deploy the old 42 entries as faithful.
Repeated `_a`/`_b` entries require their own property/budget verification even
when they share a selected waveform. The candidate does not fix mixer balance,
Bink streams, or native cues outside this bounded inventory.

## October 14 — review-only follow-up

User reports English radio just after the crash. Read-only native inventory finds
17 `VI1_*` dialogue/radio identities in `SP_VI2_kismet01_LOC_INT`; all17 have
existing local pack entries/audio. Fourteen are represented by the original
SIDE29 candidate, while three are in the earlier text-reviewed vertical slice.
No genuinely absent native radio text was identified in that decoded bank.
This is coverage evidence, not proof which event/package played in the session;
MAIN still needs runtime identity evidence and integration.

The original SIDE29 remains immutable. A separate follow-up has eight distinct
selected takes from the nine review targets, with mono PCM16/24k/hash/timing
PASS; four pass the conservative final-fit ASR screen and four remain REVIEW.
Nimitz/name and callsign pronunciation still require listening. The ninth,
reload, is excluded: guarded full-word and shorter takes require tempo above
2.13, violating the1.85 limit. No gate is relaxed to include it.

The existing intro-reference generator's explicit100-token/four-EOS-frame
settings were reused, rather than installing a new model or changing profiles.
Source and final-fit ASR differ for the right-direction bark: the source is
recognized correctly, the old silence filter fit is not, and a contiguous PCM
trim with40ms boundaries is again recognized correctly. That establishes
boundary/ASR sensitivity, not that every previous line lost words. New selected
takes use the conservative guard; tests prove interior PCM and faint edge
samples remain a single contiguous source slice. Excessive durations reject.

`export_native_voice_handoff.py` independently verifies each selected WAV and
exports `sha256` and `selected_wav_sha256` as its current identity. Historical
`candidate_wav_sha256`/input hashes remain only in a nested provenance object;
they must never substitute for the generated/selected file hash. Every native
binding is checked against original text, property bytes/hash, rate and frame
budget. Seven synthetic tests and syntax/diff checks pass; all evidence remains
offline, with no Steam writes/launches or auditory approval.

Private handoff: `deployment/hands-dubbing/native-review-handoff-v2-20261014/`,
with `selected.json`, `native-bindings.json`, `coverage.json` and explicit
`native-encoding-gates.json`. Eight selected
WAVs and raw/fitted ASR are separately retained under
`deployment/hands-dubbing/native-review-improved-20261014/`. No whole1886-pack
regeneration, mixer/texture/renderer modification or native playback claim.

Native encoding remains MAIN-owned and unverified by SIDE. MAIN reports an
old-take Vorbis experiment for `VI1_0107_CP2` with nominal38556frames but
independent decode38428frames. That does not establish the result for this
distinct new take, but is an explicit integration gate: decoded rate/channels
and count must exactly equal the unchanged native property budget. Do not infer
codec-delay compensation, crop speech or modify original properties to pass.
