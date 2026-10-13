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
