# Full dubbing status — 2026-10-08

## Completed

- Generated `894/894` player-facing Polish WAV entries from the current localization draft.
- All files are in `audio/polish/full_voicepack/`.
- `manifest.jsonl` preserves the original localization id, Polish text, selected profile and generated filename.
- Extracted all `13,502` single-player English `SoundNodeWave` assets from `SP_*_LOC_INT.xxx` into `audio/story_en_reference/`.
- Indexed those assets in `audio/story_en_reference/soundnodes_manifest.csv`.
- Transcribed `1,093` story candidates with local English ASR; `992` non-empty lines were retained for voice generation in `audio/story_en_reference/story_transcripts.jsonl`.
- Generated `992` Polish machine-translation WAVs in `audio/polish/story_voicepack/`, with speaker-profile routing and source/generated duration metadata in `manifest.jsonl`.
- Added a second story translation pass with local `facebook/nllb-200-distilled-600M`; its 992-line candidate pack is in `audio/polish/story_voicepack_nllb/`.
- Ran full Whisper QA over the final 1,886-line coverage and stored per-line language, ASR, similarity, timing and review fields in `research/localization/polish_dubbing_qa_final.json`.
- Controlled testing proved shared Pocket-TTS conditioning caused cross-line language drift. Failed lines were regenerated with isolated states, then 1,118 remaining failures received an explicit, separately recorded Piper Polish fallback attempt. Earlier audio remains under each pack's `qa_attempts/` directory.
- The remaining listening work is prioritized in `research/localization/POLISH_DUBBING_REVIEW_QUEUE.md` and the machine-readable `polish_dubbing_review_queue.json`.

## Boundary that remains

The story voice pack is a generated working set, not a finished localized release. Its Polish text comes from local machine translation and is marked `machine_draft`; it needs human correction, especially for names, idioms and long log entries. A targeted correction pass fixed and regenerated 76 mixed-language localization lines, with the previous WAVs preserved under `audio/polish/full_voicepack/qa_attempts/piper_before_remaining_corrections/`. The consolidated report now contains `313` PASS and `1,573` REVIEW_REQUIRED lines across all 1,886 entries, with no remaining automatic hard failures. Review status is intentional: story lines remain machine-translated, and short names, numbers, inflected words and longer UI descriptions still require listening review. No invented dialogue is attached to those assets.

The generated 894-file pack is a complete voice working set for the text currently available. It is not yet a replacement package or runtime redirect; original game files remain untouched.
