# PS label rewrite (Scaleform/GFx) — design + mapping

Branch `feature/ps-prompt-icons`. The Controls menu labels ("Enter", "Esc",
"Left Shift / **Pad 9**", …) are **Scaleform/GFx text** from
`RavenShared.RUIDataStore_StringAliasMap` (PC set), not textures — the injector
can't touch them (its log shows the `360_*` glyph textures are never bound).

This header is the pure mapping half of the rewrite; the mod supplies the hook.

## Files

| file | contents |
| --- | --- |
| `spikes/view_matrix/prompt_label_rewrite.h` | `IconForLabel(text)` → PS icon name (longest substring wins), tunable `LabelTable` |
| `spikes/view_matrix/prompt_roles.h` | action → icon name |
| `spikes/view_matrix/prompt_atlas_generated.h` | icon name → atlas UVs (20 PS icons) |
| `assets/processed/ui/runtime/vr_prompt_atlas.bmp` | the icon atlas |
| `tools/reference-tests/ui/prompt_label_rewrite_test.cpp` | mapping test (PASS) |

`IconForLabel` handles the labels seen in the Controls menu: action rows
(`Sprint`→L3, `Age`→Square, `Impulse`→R1, `Anti-Gravity`→R3, `Use Health`→DpadUp,
`Chrono Ping`→R2) and the key/pad column (`Enter`→**Cross**, `Esc`→Circle,
`Reset`→Square, `Pad 5/6/9/10/11/12`→Square/R1/L3/R3/DpadUp/R2). Unknown labels
return `nullptr` and stay as text. **The `Pad N` rows are the game's DirectInput
pad numbering — tune `LabelTable` after one look at the menu.**

## Recommended hook (main agent)

The mod already hooks native UE3 functions by name/index (the `FireAmmunition`
seam uses `FnIdx`/`NameFromIndex`). Use the same machinery for one of:

1. **Alias-set override (preferred, least rendering work).** Hook
   `UIDataStore_StringAliasMap` resolution (`GetStringValue` /
   `HasAliasMappingForPlatform`) and return the **PS3** entry for the requested
   key. If the PS3 entries are button glyphs in the UI font, the menu then shows
   PS symbols with **no drawing code at all**. Verify first: dump one PS3 label
   and see whether it is a glyph char or a word.
2. **GFx text rewrite.** If the PS3 set is still words, hook the GFx text setter
   (or the alias resolver) and, for a label that `IconForLabel` matches, replace
   the string with a sentinel; then the overlay (below) draws the icon.

## Overlay fallback (guaranteed visible)

If neither seam can be placed, draw the icons over the menu with the existing
overlay plumbing (`vr_menu_overlay.h`: `g_vrMenuTexture` + plane + the prompt
atlas loader). Per row: `IconForLabel(rowText)` → `kVrPromptIconUv` →
`DrawVrPromptIcon`. This needs the row's screen position, which is the hard part;
the safest subset is the **bottom bar** (`Enter/Select`, `Esc/Back`,
`Reset`) whose positions are stable.

## Scope note

This is **mod code (Scaleform/UI)**, the main agent's area — not the texture
injector. The icons + mapping above are ready; only the hook seam needs the
engine offset/name (same class of work as `FireAmmunition`).
