# Full Polish draft

The complete draft is generated locally from every `*.int` record with
`build_polish_draft.py`. Exact reviewed UI/tutorial entries come from
`ui_batch.json`; repeated instructional phrases receive a conservative draft;
ambiguous strings retain English text and are marked `needs_review`.

The current generated handoff contains 7,056 localization entries: 283 exact
translations, 667 conservative machine drafts, 6,059 entries requiring human
review, and 47 empty source values. All 59 subtitle tips are translated. The
full file remains outside Git because it contains the game's original text.

This is the safe way to cover the entire string inventory without silently
shipping untranslated or mistranslated text. The local JSONL is ignored by
Git and is not an installable game package.

```powershell
python localization-tools/build_polish_draft.py `
  C:\Users\Trilex\vr-mods\SingularityVR-localization-local\inventory\Coalesced_INT.jsonl `
  --output C:\Users\Trilex\vr-mods\SingularityVR-localization-local\polish\full_draft.jsonl
```
