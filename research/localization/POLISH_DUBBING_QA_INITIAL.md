# Polish dubbing QA

Automated full pass over both generated packs. Whisper language detection was not forced to Polish. Story translations remain machine drafts and are therefore conservatively marked `REVIEW_REQUIRED` even when audio agreement is good.

Model: `Systran/faster-whisper-base`  
Total lines: **1886**  
Audio files found: **1886**

## QA counts

- PASS: **47**
- REVIEW_REQUIRED: **617**
- FAIL_LANGUAGE: **166**
- FAIL_GIBBERISH: **1043**
- FAIL_PRONUNCIATION: **0**
- FAIL_TRANSLATION: **0**
- FAIL_TRUNCATED: **13**
- FAIL_AUDIO: **0**
- FAIL_WRONG_VOICE: **0**
- MISSING_AUDIO: **0**

## Speaker/profile counts

- `barisov_dmitri`: 184
- `demichev_ruslan`: 31
- `kathryn_tetiana`: 263
- `narrator_user`: 1407
- `renko_user`: 1

## Review focus

- All story lines are machine-translated and require human meaning/naturalness review.
- `polish_dubbing_qa.json` retains ASR text, detected language, probabilities, timing and per-line notes.
- No automatic embedding-based wrong-speaker classifier is available locally; voice mapping is checked against the manifest profile and remains a manual listening item.
