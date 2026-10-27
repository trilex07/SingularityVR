# Polish dubbing human review queue

Review items: **1573**

The queue contains only lines classified `REVIEW_REQUIRED`; generated audio remains available at each `audio_path`.

## Priority counts

- P1_story_translation_and_names: **992**
- P1_localization_language: **505**
- P1_localization_audio: **76**
- P2_short_or_inflected_ui: **0**

## Review order

1. Story translation, names and pronunciation (`story_nllb`).
2. Localization lines with language or audio warnings.
3. Short UI commands and inflected Polish strings where Whisper is conservative.

Full per-line data is in `polish_dubbing_review_queue.json`.
