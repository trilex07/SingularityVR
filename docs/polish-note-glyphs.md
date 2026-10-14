# Polish note glyphs — SIDE result

Offline only. No Steam deploy, no game launch, no physical verification.
Branch `fix/polish-note-glyphs`. MAIN owns the headset check.

## What SPNotepad actually uses

`SPNotepad` is `RvUIGFxMovie_Notepad` inside `UI_SinglePlayer_SF.xxx`.
Its movie is `SPMenus`. That movie sets `bUsesFontlib` and does not embed
the note face. ImportAssets2 (tag 71) imports four faces from `gfxfontlib.swf`:

| import | character id |
| --- | --- |
| WFontHeader | 11 |
| WFontButton | 15 |
| WFontTitle | 19 |
| WFontNote | 278 |

`WFontNote` is `TypekaMix,Normal` (GFxUI.int). The cooked library is the
single `fonts_en` export in `ui_GFx_fonts.xxx`: DefineFont3 id 11, 254 glyphs,
wide offsets and wide character codes. The note body is DefineEditText
character **279** in `SPMenus`:

- font 278, height 400 twips, color black
- HTML, multiline, word wrap, read-only, no auto-size
- bounds 3960×3960 twips, leading 40
- initial text `<p align="left"></p>`, empty variable name

Runtime text is written into that field. Button labels in the same movie use
`WFontButton`, not TypekaMix. This patch does not open `UI_SinglePlayer_SF.xxx`.

## Missing outlines, not a bad code page

TypekaMix already has code points for `ĄĆĘŁŃÓŚŹŻąćęłńóśźż`. `Ó` and `ó` are
real outlines (985 and 834 bytes) and stay byte-identical. `Ł` and `ł` are
empty 2-byte shapes. The other fourteen Polish slots share stub outlines of
at most 34 bytes. Their advances are the placeholder 10240, while `Á` keeps
`A`'s advance and `Ç` keeps `C`'s.

`PolishUtf8ToNoteWide` in MAIN's working tree turns UTF-8 into UTF-16, then
CP1250, then UTF-16 again. Letters that exist in CP1250 come back as the same
Unicode characters. That path cannot draw a stub. This branch does not change
the DLL.

## Patch

`tools/fonts/patch_note_font.py` accepts only this uncompressed package:

- input SHA-256 `3f7bf0bef08d73aed3b1ba249730b4bbb16ec427dd49992a6f4c4a7ce6c8b51b`
- output SHA-256 `60e7287185300caa2a8e64fbe14b741475d4d959944b6b0e51f84547841e8db8`
- 491520 bytes in, 507904 bytes out, still uncompressed v584, one `fonts_en` export

It rebuilds sixteen TypekaMix shapes from outlines already in that font:
acute from `Á`/`á`, dot from the period, bar from the hyphen, and the hook
under `Ç`/`ç` mirrored onto the right side as the ogonek. Advances of those
sixteen slots become the base letter's advance. The code table, kerning tail
(10026 bytes), zero glyph-bounds table, `Ó`/`ó`, ASCII, digits and every other
DefineFont3 stay byte-identical. Tag length, SWF length, RawData count and
export size grow with the font. The tool refuses a second run, any other hash,
and any output path under Steam or Program Files.

Marks stay inside the font's ascent 18600 and descent 5400, which is the same
vertical budget `Á` and `Ç` already use, so the note field is not resized.
Wrapped lines of `Zażółć gęślą jaźń`, the eighteen letters, `[use]` plus
`LT RT A B`, and the long Katorga-12 closing note stay within the 3960-twip
field. Paragraph breaks remain.

No third-party font is embedded. The patcher reads the user's own package at
run time. Do not commit `ui_GFx_fonts.xxx`, SWF extracts or the preview PNGs.

## Reproduce

```
python tools/fonts/tests/test_note_font.py
python tools/fonts/patch_note_font.py --input ui_GFx_fonts.xxx --output C:\outside-steam\ui_GFx_fonts.xxx --report report.json --preview notes.png
```

The package test reads the installed file and writes previews only to a temp
directory. It skips if that file is absent. Previews are `notes-glyphs.png`,
`notes-pangram.png` and `notes-note.png`.

## MAIN integration

1. Back up the installed `RvGame\CookedPC\ui_GFx_fonts.xxx` and check the
   input hash above. Leave `UI_SinglePlayer_SF.xxx` untouched.
2. Run the patcher. Confirm the output hash above. Copy that file over the
   installed one only for a notes check in the headset.
3. Look at a note that contains `Zażółć gęślą jaźń` or the closing-down
   paragraph. Acutes, ogoneks, strokes and dots should be visible, lines
   should wrap, and button prompts elsewhere in the menu should be unchanged.
4. Restore the backup unless that check passes. This result is
   `OFFLINE_PASS_PHYSICAL_PENDING`. It is not a physical pass.
