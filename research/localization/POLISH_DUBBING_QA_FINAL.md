# Polish dubbing QA — final coverage

Full inventory: NLLB story pack + localization pack. Automated Whisper checks are conservative; short low-confidence commands, names and inflected UI strings remain `REVIEW_REQUIRED` rather than automatic hard failures.

Total lines: **1886**
Audio files found: **1886**

## Final counts

- PASS: **313**
- REVIEW_REQUIRED: **1573**
- FAIL_LANGUAGE: **0**
- FAIL_GIBBERISH: **0**
- FAIL_PRONUNCIATION: **0**
- FAIL_TRANSLATION: **0**
- FAIL_TRUNCATED: **0**
- FAIL_AUDIO: **0**
- FAIL_WRONG_VOICE: **0**

## Findings

- Shared Pocket-TTS conditioning caused cross-line language drift; isolated conditioning was used for retry generation.
- NLLB replaced the first story translation model for the 992-line candidate pack.
- 1,118 failed lines received explicit Piper fallback attempts; originals remain under `qa_attempts/`.
- 76 mixed-language localization lines were corrected and regenerated with Piper; the previous WAVs remain in `qa_attempts/piper_before_remaining_corrections/`.
- Short commands, proper names, numbers and inflected Polish UI strings with understandable ASR remain listening reviews instead of automatic failures.
- The 1,573 review items are prioritized in `POLISH_DUBBING_REVIEW_QUEUE.md` and `polish_dubbing_review_queue.json` (992 story lines, 581 localization lines).

Exact remaining hard-fail IDs: `polish_dubbing_remaining_failures.json` (0 rows).

## Status

**CONTINUE DUBBING QA** — the report still contains automatic failures and review-only lines.
