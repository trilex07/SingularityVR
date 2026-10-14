# SIDE — Polish glyphs in SingularityVR notes

## Objective

Repair Polish characters in the native notes UI, preserving the game's font,
styling, line breaks, layout and accepted VR menu presentation. Do not replace
Polish letters with ASCII substitutes. This is font/rendering work, not TTS.

User report: notes lack Polish glyphs. The latest native session reached
`SPNotepad`, class `RvUIGFxMovie_Notepad`. Runtime code logged the proposed
CP1250 path, but the physical result still failed. Conversion is not proof that
the font contains glyphs.

## Ownership and starting point

- SIDE: dedicated branch `fix/polish-note-glyphs` and isolated worktree. Read
  `AGENTS.md`, `STATUS.md` and applicable skills first. Never modify another
  checkout, merge main, launch Steam, deploy, or restore installed files.
- MAIN's current checkout is
  `C:/Users/Trilex/vr-mods/SingularityVR-post-opening`, branch
  `fix/post-opening-followups`, committed HEAD `f2b9c23`. PR #19 is draft.
  It has uncommitted changes from a temporarily substituted MAIN model;
  inspect read-only, do not assume they are on main or copy them blindly.
- Relevant committed inputs: `tools/audio/first_area_notes_pl.json`,
  `tools/title/swf_title_patch.py`, `tools/title/tests/test_swf_title_patch.py`,
  `tools/title/prepare_title_package.py`, and `PolishRewriteFString` in
  `spikes/view_matrix/d3d9.cpp`. Inspect the newer Notes conversion read-only.
- Reuse existing UE3/LZO and GFx/SWF tooling and original-package backups.
  Work on private copies. Do not redistribute game SWFs, packages or fonts.

## Evidence before changing a font

1. Identify the actual SWF/movie, text field, font ID/name and embedded font
   tags or glyph atlas used by SPNotepad. Do not assume another menu's font.
2. Trace the text through localization -> FString -> GFx -> glyph lookup.
   Distinguish missing glyphs from decoding, substitution, wrapping or clipping.
   The current UTF8 -> CP1250 -> Unicode round trip does not create glyphs;
   establish the actual encoding contract rather than repeating this guess.
3. Inventory the font's character map for
   `ĄĆĘŁŃÓŚŹŻ ąćęłńóśźż` and ordinary punctuation. Preserve existing ASCII,
   digits, controller/input tokens, advances, kerning and baselines.
4. Choose the smallest supported fix: correct proven decoding, extend the
   affected embedded font/atlas, or use a correctly scoped compatible font.
   Preserve the original visual identity and document font licensing.
5. Supply a reproducible patcher that transforms the user's own package,
   checks supported input hashes, and rejects unknown formats. Repair SWF tag
   lengths and UE3 package compression; no arbitrary byte overwrites.

## Offline validation / delivery

- Test `Zażółć gęślą jaźń`, all upper/lower Polish letters, and at least one
  real long note with paragraphs and input tokens.
- Provide offline rendering screenshots where tooling supports them; verify
  accents, bounds, clipping, wrapping and no loss of existing glyphs.
- Include deterministic tests, untouched repack control, input/output hashes,
  original -> control -> candidate round trips, and exact scope of changed
  tags/resources. L1/L2 only; MAIN owns L3/L4 and Steam backup/restore.
- Commit and push tools, tests and concise evidence only. Hand off branch/SHA,
  artifact hashes, reproduction command, tested font coverage and remaining
  limits. Do not claim physical verification.

