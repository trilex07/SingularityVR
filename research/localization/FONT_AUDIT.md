# Polish glyph audit

Status: **UNKNOWN — runtime font coverage still requires an export test.**

The game ships `ui_GFx_fonts.xxx` (491,520 bytes) and
`ui_GFx_fonts_jp.xxx` (2,621,440 bytes). No `POL` or `PLK` locale package was
found beside the `INT`, `ESN`, `FRA` and `ITA` resources. The presence of the
Japanese font package does not prove that Polish glyphs are available in the
Latin GFx font.

The required test is:

* decompress the font package on a disposable copy;
* export the GFx font glyph ranges with UEViewer/UE3 tooling;
* verify `ą ć ę ł ń ó ś ź ż` and uppercase variants in the actual font used by
  `UI_SinglePlayer_SF`;
* render a short phrase containing every glyph and inspect fallback boxes;
* only then decide whether a font asset extension is needed.

The Polish fixture is UTF-8 and uses Polish characters, so the translation
validator catches encoding failures early. No font asset has been changed.
