# Polish localization work area

The first batch is a reviewable fixture for menu, pause, tutorial and small
dialogue strings. It is not an installable game package. All input references
remain symbolic (`[fire]`, `[grab]`, `[use]`, and so on), so the VR runtime can
choose the correct PSVR2 prompt without baking controller glyphs into text.

Run the validator from the repository root:

```powershell
python localization-tools/validate_translation.py localization/pl/ui_batch.json
```

The source inventory and any generated package-sized data stay under the
ignored `SingularityVR-localization-local` directory. This branch contains
tooling, IDs, hashes, glossary entries and small fixtures only.
