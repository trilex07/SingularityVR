# Native Start Game / Quit Game button polish — SIDE offline handoff

Branch `ui/main-menu-button-polish`; MAIN owns Steam deployment and PSVR2 acceptance.

## Exact producer and change

The first Start Game / Quit Game menu uses the native `SPMenus.Button` sprite (character 104) through `AddButtonCenter -> AddButtonBase -> attachMovie("Button")`. `SPMenus.ButtonLeft` (155) is the pause layout; `iis.Button` (8) is separate. The accepted V2.1 package already embeds four metal plates. The main button's native editable text fields 99/102/103 use centered alignment, 24 px font height, zero left and right margins, and approximately 320 × 31 px bounds. The plate fills those native bounds. The zero text margins and large font leave the visible text cramped; scaling the whole VR menu would change the unrelated pointer/layout presentation.

`tools/title/polish_main_menu_buttons.py` starts from the exact private, already remastered input SHA-256 `9482BCC00BA91377E5D7AECC18DC3EE35EA1ED37BBD0BEE4C696D19F7D8ACC9A`. It changes only `SPMenus.Button` text definitions 99/102/103 (font 480→420 twips, symmetric margins 0→300 twips = 15 px) and embedded bitmap definitions 358–361. The new 480 × 54 plates have graphite metal, narrow technical edges, a restrained cyan hover, recessed amber press, and subdued disabled state. Their existing DefineShape bounds, mappings, the 40-frame timeline, text field rectangles, placement matrices, actions, state labels and mouse hit regions are unchanged. The native `AddButtonBase` `_height` progression and laser/R2 event ownership are therefore preserved structurally. No additional D3D9 draws or runtime code are added. `iis`, `ButtonLeft`, Settings, pause and station artwork are unchanged.

Source localization records map `RvUI|StartGame` to EN “Start Game” / PL “Rozpocznij grę” and `RvUI|QuitGame` to EN “Quit Game” / PL “Wyjdź z gry”. The longest label, “Rozpocznij grę”, measures 143 px in a 21 px Arial proxy against 290 px available after symmetric margins. This is a layout proxy; the imported native font's final rasterization and diacritics must be checked in PSVR2.

## Private deliverables and reproduction

Private candidate: `C:\Users\Trilex\vr-mods\SingularityVR-main-menu-polish\build\menu-polish-private\candidate-v3\UI_SinglePlayer_SF.xxx` (SHA-256 `CDA63308054F63B14496CBDF95989CB4F73A303082559288F08E8FA43FCA47EF`). Its `manifest.json` records input, decoded, menu and candidate hashes. Synthetic before/after sheet: `C:\Users\Trilex\vr-mods\SingularityVR-main-menu-polish\build\menu-polish-private\before-after.png`. No private package or game-derived GFx movie is committed.

Reproduce with the pinned private source and decoded `current.u` using `python tools/title/polish_main_menu_buttons.py --original <private V2.1.xxx> --logical <decoded.u> --decoder <unpack_chunks.exe> --out <new private directory>`. Unknown hashes and existing output directories refuse. The output package is independently decoded and compared byte-for-byte against the generated logical package.

## Validation and remaining gate

- **PASS offline:** 45 UE3 exports retain identity; `iis` movie byte-identical; all `SPMenus` tags except the three text definitions and four bitmap definitions have identical payloads; every native timeline and AVM1 action is unchanged; text boxes and hit geometry are unchanged; four state bitmaps are distinct; decoder roundtrip is byte-exact.
- **PASS offline:** PL/EN 21 px font proxy fits 15 px symmetric margins; synthetic comparison shows no clipping and state contrast. Title test discovery: 25 tests, 22 passed and three private fixture tests skipped; the new test directly validates this private package.
- **PASS offline:** frontend comfort and context cursor suites. `test-menu-polish.ps1` compiled and ran its native C++ fixture, then failed its pre-existing `FrontMenuScalePct=72` assertion because the current MAIN baseline INI is `100`. This branch does not change that accepted runtime setting; the script's static assertion is stale for this baseline. This failure is unrelated to native package bytes and is not reported as a full suite PASS.
- **PENDING MAIN:** actual imported GFx font glyph coverage/centering, stereo readability, hover/press/disabled appearance, laser and R2 behavior, and first-menu Start/Exit action in PSVR2. These cannot be claimed from the synthetic comparison or structural test.

MAIN acceptance: back up installed DLL/UI with hashes; deploy the exact candidate package through the guarded installer; view Start/Exit in both eyes in EN and PL; inspect all four native states, the longest label and Polish `ę`/`ź`; click each button with laser/R2, confirm no shifted hit target or changed navigation; check pause, Settings and stations; restore the verified backup on any failure. SIDE did not access or modify Steam.
