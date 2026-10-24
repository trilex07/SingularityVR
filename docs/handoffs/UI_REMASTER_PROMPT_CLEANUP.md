# UI V2.1 + legacy prompt cleanup — SIDE handoff

Base: `279203ba` (PR #58 candidate plus physical-acceptance/status record).
Branch: `feature/ui-remaster-legacy-prompt-cleanup`. No Steam writes or game launch.

## Evidence and ownership

Read-only installed-file audit found all three correct atlases, with UiRemasterRuntime/Settings/Wheel=1 and VrPromptTokens=1. Consequently, missing installation is NOT the proven explanation for an old-looking wheel/settings. File identity is not a runtime draw receipt. The existing renderer uses exactly `Binaries/vr-assets/ui/remaster/`, and detailed-icon UVs already match the neutral atlas. Added bounded `M02 wheel binding` receipts: result=1 means the existing two-eye composite committed; zero is fallback. Inspect these before diagnosing a new visual failure.

Main/pause had no V2.1 atlas backdrop route. Settings and wheel already had working offline-tested M02 P0/P1/P2 routes. The old prompt label/role tables contain historical guessed DirectInput mappings; cleanup does not use them. Existing session log proves C++ Localize producer `section=Keyboard key=Button10 package=RvGame source="Pad 11"`, and Button2 -> Pad 3. Other source hints contain real `[age]`/`[reload]` tokens.

Hypothesis/falsifier: exact Keyboard/ButtonN label cleanup plus action-token rewriting removes those aliases without deleting native sentences. A Pad label from another producer falsifies completeness; capture its section/key before extending the allowlist. No substring screen mask or all-GFx suppression is used.

## Connected

- Enabled existing settings/wheel skins as source defaults; retained explicit OFF switches and missing-asset fallback. Installed files were already enabled, so this is not presented as their root-cause fix.
- Main and pause reuse V2.1 `terminal_slate_neutral` behind the existing projected native quad, with a 3.5% decorative perimeter. Native quad, pointer hits, text, scroll and menu positions are unchanged. Main composition uses the existing pixel/state rollback transaction; pause keeps its native mandatory draw even if decoration fails. Modal dialogs, notes, stations and VR Settings are excluded from this backdrop path.
- This is an atlas-backed **frame/backdrop**, not a replacement of native GFx button art or a second menu. Opaque native capture pixels can cover the interior; do not claim every native button has been reskinned. Replacing individual GFx widgets needs proven widget identities, not guessed masking.
- `VrHideLegacyPadPrompts=1` is VR-only. Exact `Keyboard/Button<digits>` fields whose whole value is `Pad <digits>` become empty FStrings (Num=1). The surrounding native instruction remains. It does not infer action semantics from Pad numbers.
- Known action tokens use a bounded once-per-second snapshot from `xrEnumerateBoundSourcesForAction`/`xrPathToString` after successful input sync, plus current native-mask routing and reload-layout remaps. No new bindings or input writes. Unknown/ambiguous sources show `VR`, never a guessed button. Unknown action tokens remain unchanged. Notes, subtitles and objectives retain their text; exact known tutorial Reload/Jump/Crouch fields can replace an embedded legacy label.
- Text labels `(O)/(X)`, L2/R2, grip/stick and localized Square/Triangle names avoid requiring new GFx font glyphs. Existing M02 Sense atlas glyphs remain in VR Settings. Historical fixed gameplay legends are disabled under the new cleanup; OFF restores the previous prompt path, including its VrPromptTokens setting.
- Weapon-mounted HUD 3D, TMD poses, firing, inventory, haptics and localization content untouched.

## Required assets (unchanged)

Copy/review only through MAIN tooling if hashes differ. Source directory: `assets/processed/ui_remaster/atlas/`; destination: `Binaries/vr-assets/ui/remaster/`.

- `ui_atlas_neutral.png`: `3EF1782696B4A5E1E9D93F409C5B0F6FF014B78CC42FAB06EC29110661981818`
- `ui_atlas_localized_pl.png`: `6436AA257F310F65A1F65C04D7435D9BF8343BFEFAB2EEE50A8A836F96FC83FD`
- `ui_atlas_localized_en.png`: `53B570C4787B834270CBCF0943622877613A7EE7E2AD07FA6A363DFE11A422BA`

All three installed hashes matched at this audit. `tools/audit_ui_remaster.py` can repeat the read-only inventory. No PNG regeneration or private content added.

## INI delta for MAIN

Under `[Render]`: `UiRemasterRuntime=1`, `UiRemasterSettings=1`, `UiRemasterWheel=1`, **`UiRemasterMenus=1`**, **`VrHideLegacyPadPrompts=1`**. New flags default 1; existing explicit zero settings still win. Disable UiRemasterMenus for exact old menu backdrop; disable VrHideLegacyPadPrompts for exact old prompt policy. Preserve all unrelated installed INI fields. No full INI replacement.

## Validation / limits

PASS offline: `tools/test-ui-cleanup.ps1` (12 programs), selective prompt classification/PL-EN/control paths, settings laser layout, wheel selection/geometry, main/pause policy. D3D9 and D3D9Ex fixture exercises production backdrop helper, both-eye pixels, unchanged input quad, texture/blend restoration on success and injected draw failure. Atlas QA: 163 sprites; detailed weapon icons and unchanged localized atlas pixels PASS. Critical gameplay/TMD/reload/WeaponLocker/OLED suite PASS; HUD 3D: 120 D3D9 + 120 D3D9Ex pixel cases PASS. Production x86 PE32 `/O2`; exact final hash and source identity in `UI_REMASTER_PROMPT_CLEANUP.json`.

These are not gameplay or headset results. Native GFx readability over the new backdrop, active runtime-bound controls (including user rebindings), and full prompt coverage remain physical/runtime PENDING. Unrecognized producers are deliberately left unchanged. Unknown source path is not translated from a guessed Pad number. Fonts are not patched.

## One MAIN PSVR2 procedure

Start from accepted profile, apply only the reviewed DLL and UI delta. Without chat switching: inspect main menu and pause; open VR Settings in PL then EN and use laser/R2 plus scrolling; open R3 wheel and select all acquired guns; inspect one reload/use tutorial and a note/subtitle. Require readable native text, correct icons and hits in both eyes, no Pad 11/3 at the proven producers, no duplicate text, and unchanged 3D ammo HUD/TMD. Close normally. Collect `M02 wheel binding`, `M02 menu skin`/`frontend-clean M02skin`, and token rewrite receipts. If a label remains, record its exact producer instead of suppressing broad text. MAIN owns deployment, rollback and physical acceptance.
