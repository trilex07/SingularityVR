# Polish dubbing workflow

## What can be automated

- package inventory and export listing;
- mapping voice IDs to scene/localization records;
- Polish script preparation from the localization draft;
- loudness normalization, silence trimming and duration checks;
- A/B playback against the English reference.

## What still needs voice material

A finished Polish dub requires either Polish actor recordings or an explicitly approved synthetic voice set. I will not claim the English files have been translated into Polish voices until actual Polish takes exist and have been checked against the source timing.

## Recommended first batch

Start with the first story chapter's localized scene packages and the radio/captain dialogue. This gives a small test set for extraction, timing, encoding and playback before scaling to all chapters. Keep original English files untouched as references.

## Delivery layout

```text
audio/polish/source-en/       # original reference metadata only
audio/polish/script/         # Polish lines, IDs, timing, speaker
audio/polish/recorded/       # dry Polish takes
audio/polish/mixed/          # edited and normalized takes
audio/polish/redirect/       # runtime test assets
```
