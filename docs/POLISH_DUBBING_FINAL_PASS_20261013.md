# Polish dubbing final pass — 2026-10-13

This pass keeps `full_revoice_intro` frozen and writes a separate, reversible
candidate. It does not modify Steam files or commit generated audio.

## Work completed

- Rechecked the complete 1,886-row candidate against its English source, Polish
  script, synthesis input and prior ASR results.
- Repaired the 146-row review queue: 125 flagged rows received corrected text
  and new audio; 21 were retained because their Polish text is already faithful
  and the remaining alert is not evidence of a translation defect (examples
  include short exclamations, proper names, UI placeholders and acronyms).
- Two additional Devlin radio rows had passed the initial ASR screen but were
  incomplete/miscast. Those were corrected too, for 127 regenerated recordings
  total.
- Reassigned all 16 lines that identify their speaker as Devlin 4-1 from the
  narrator fallback to the existing `devlin_mykyta` Polish voice reference.
  Narrator lines continue to use the user's approved intro reference.
- Fixed synthesis input that rendered key aliases as ungrammatical Polish and
  split newline-delimited tutorials into separate utterances. This prevents a
  list from being synthesized as one overlong chunk.
- Removed only measured low-level pre-roll in 16 new files, then repeated the
  base ASR and format checks. The final base screen has no technical or
  possible-truncation flags in the replacement set.

## Automated QA

| Scope | Result |
|---|---:|
| Full staged corpus, base Whisper | 1,840 / 1,886 pass; 46 remain flagged |
| Original 146-row queue, base Whisper after repair | 102 pass; 44 flagged |
| Original 146-row queue, independent small Whisper | 119 pass; 27 flagged |
| Original queue passing both models | 98 / 146 |
| Priority listening set | 48 candidate clips, 4m 21s |
| Complete A/B reel | 146 pairs, 29m 10s |
| WAV validation | 1,886 / 1,886 readable 24 kHz mono PCM16 files |

The 48 clips are ASR review cues, not confirmed defects. They mostly contain
character names, call signs, numbers, Russian/Ukrainian-accented character
references, very short utterances or menu aliases. One small-model heuristic
still marks the short `ImpulseRocks` line as possible truncation because it
recognized the Polish audio as another language; the base model did not flag
that file. This item is included in the listening set.

ASR and waveform checks cannot confirm acting, naturalness, pronunciation
quality or voice identity. Perceptual QA is therefore **pending**. No line is
claimed to have passed human listening in this pass.

## Candidate and listening files

Full local staging directory:

`audio/polish/repair_candidate_20261013/full_revoice_final_pass/assembled_reviewed/`

- Voicepack: `voicepack/` (1,886 WAVs, manifest, QA and six conditioning references)
- Short focused review: `review_priority.wav`, indexed by `review_priority.md` and `.json`
- Full before/after review: `review_ab_146.wav`, indexed by `review_ab_146.md` and `.json`
- SHA-256 list: `voicepack/SHA256SUMS.txt`
- Assembly summary: `ASSEMBLY_REPORT.json`

The focused reel is the practical first listen. A means previous Polish audio;
B means this candidate in the full A/B reel. The shorter priority reel contains
candidate clips only. Nothing here has been installed into the game.

## Integrity

- Full candidate manifest SHA-256: `f3e4a6693b7a9d4e6cc052372734398619a021744ad2f77898635a7b84972b1e`
- WAV checksum list SHA-256: `f4de3bea5d62b7f324ec480570e6aa0381240435a61ebc0745829e9d40669e8f`
- Focused listening WAV SHA-256: `c47b58fccf9d4138f5a404c59d9d580942904e380da3337f7048245048700187`

The generated WAVs, source transcripts, local correction maps and candidate
manifests are ignored by Git. Only the reusable generator changes, offline
assembly/QA scripts and this summary are intended for version control.

## Reproduction

Use the local Pocket-TTS Polish 6L runtime and the already-staged references;
no external generation service is used. The exact row lists and corrections
are local-only files under `full_revoice_final_pass/`.

```powershell
py tools/audio/generate_intro_narrator_full_candidate.py `
  --output-dir audio/polish/repair_candidate_20261013/full_revoice_final_pass/replacements `
  --ids-file audio/polish/repair_candidate_20261013/full_revoice_final_pass/regenerate_ids.json `
  --text-overrides-json audio/polish/repair_candidate_20261013/full_revoice_final_pass/text_overrides.json --force

py tools/audio/generate_intro_narrator_full_candidate.py `
  --output-dir audio/polish/repair_candidate_20261013/full_revoice_final_pass/replacements `
  --ids-file audio/polish/repair_candidate_20261013/full_revoice_final_pass/devlin_profile_ids.json `
  --text-overrides-json audio/polish/repair_candidate_20261013/full_revoice_final_pass/text_overrides.json `
  --profile-overrides-json audio/polish/repair_candidate_20261013/full_revoice_final_pass/profile_overrides_devlin.json --force

py tools/audio/qa_repair_candidates.py --candidate-dir audio/polish/repair_candidate_20261013/full_revoice_final_pass/replacements --asr-model base
py tools/audio/trim_candidate_noise_floor.py audio/polish/repair_candidate_20261013/full_revoice_final_pass/replacements
py tools/audio/qa_repair_candidates.py --candidate-dir audio/polish/repair_candidate_20261013/full_revoice_final_pass/replacements --asr-model base
py tools/audio/qa_repair_candidates.py --candidate-dir audio/polish/repair_candidate_20261013/full_revoice_final_pass/replacements --asr-model small
py tools/audio/assemble_full_revoice_candidate.py --output-dir audio/polish/repair_candidate_20261013/full_revoice_final_pass/assembled_reviewed
py tools/audio/test_full_revoice_text.py
```

Assembly refuses to overwrite an existing non-empty output directory. It links
or copies the frozen base and replacements into a new staging tree and checks
all 1,886 WAV headers before reporting success.
