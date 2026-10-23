# M02 P1: VR Settings atlas styling (SIDE / offline)

Based on `df17461`. This is an independently gated extension of the unaccepted first slice, not authority to deploy or merge.

`[Render] UiRemasterSettings=0` defaults OFF. Set `UiRemasterRuntime=1` as its foundation and `UiRemasterSettings=1` only in a controlled MAIN candidate. The base panel and all existing laser/R2 hit rectangles remain authoritative. Setting P1 back to 0 returns to the first-slice visuals; setting both to 0 returns to the accepted renderer.

P1 draws a neutral header, selected-row frame and Sense R2 glyph plus the page's localized tab and footer from PL/EN atlas. The tab/footer texture follows the current Polish-language setting; existing dynamic row text remains produced by `PanelRowText`. Native title, rows, GFx interaction and controller behavior are not replaced. This is visual styling, not a full rewrite of every settings string.

Source atlases: `assets/processed/ui_remaster/atlas/ui_atlas_neutral.png`, `ui_atlas_localized_pl.png`, `ui_atlas_localized_en.png`. The runtime expects these three files at `Binaries/vr-assets/ui/remaster/`. The generated sprites and their UVs come from the existing atlas manifests. Assets are not proprietary extracted game content.

SHA256: neutral `E4DC0E7D209BDDF23816F101177C8A8B02AA24ED766C55C446877956CAC4F23B`; PL `6436AA257F310F65A1F65C04D7435D9BF8343BFEFAB2EEE50A8A836F96FC83FD`; EN `53B570C4787B834270CBCF0943622877613A7EE7E2AD07FA6A363DFE11A422BA`. P1 build DLL: `5EA585C5C797C013C2390C7554B974500D7A27264681D32CC59B43C97063C4CC` (x86 PE32 `/O2`).

The two localized textures load lazily once per language. Missing or corrupt assets latch failure until Reset, preserving the original settings rows and highlight. All D3D9 state is captured and restored around the decoration; Reset releases all three textures. The CPU layout fixture covers both eyes at 5264x2684; the D3D9/D3D9Ex fixture draws the plate, active-row frame and tab in both eyes and checks equal pixels and restored blend/texture state. Neither fixture proves headset appearance.

MAIN acceptance, only after its first-slice gate: open VR Settings in Polish and English, move laser over rows and select with R2, visit root/Display/TMD subpages, check both eyes and text legibility, switch language, close/reopen, Reset, and verify no extra panel or state leak. Archive the full log and per-eye screenshots. Do not mix with wheel acceptance.
