# Final visual integration handoff — 2026-10-26

Base: `71728b5d7e2712d1640fa18fa58e48bd6dc708ee`. Branch: `side/final-visual-20261026`.
SIDE only: no Steam files read or modified, no game launch, deployment or physical PASS.

## UI inventory and actual connections

The base already contains the UI cleanup implementation. `d3d9.cpp` loads the neutral and selected PL/EN atlas, renders Settings plates and detailed R3 wheel sprites, and connects the menu backdrop through `vr_menu_overlay.h` / `ui_menu_skin.h`. The backdrop executes **before** the native GFx menu quad. Native interaction geometry, laser handling, dynamic text, scrolling and localization remain native. This change does not alter these paths or the accepted weapon HUD 3D.

Required source files under `assets/processed/ui_remaster/atlas/`:

- `ui_atlas_neutral.png`: 4096x2048, SHA256 `3EF1782696B4A5E1E9D93F409C5B0F6FF014B78CC42FAB06EC29110661981818`.
- `ui_atlas_localized_pl.png`: 2048x1024, SHA256 `6436AA257F310F65A1F65C04D7435D9BF8343BFEFAB2EEE50A8A836F96FC83FD`.
- `ui_atlas_localized_en.png`: 2048x1024, SHA256 `53B570C4787B834270CBCF0943622877613A7EE7E2AD07FA6A363DFE11A422BA`.

Existing runtime destination is `Binaries/vr-assets/ui/remaster/`. MAIN must compare installed hashes; this SIDE source audit does not establish deployment. The neutral hash supersedes older handoffs listing `E4DC...`. Four detailed sprites are bound for Centurion (5799 vertices), Valkyrie (11387), Volk/Spikeshot (8297) and Kasimov (11389); atlas readiness alone is not proof of visible sprites.

**Full native main/pause button remaster remains BLOCKED on producer evidence.** Prepared button artwork has no proved original GFx shape/instance mapping in the present runtime. A slate drawn underneath the native quad cannot replace opaque native buttons. Existing title/mouse SWF patch tools prove only their exact leaves, not menu button identities. Do not mask broad screen rectangles, duplicate labels or claim the complete menu remaster finished. The next necessary evidence is the source GFx button shape/instance identity and its actual consumer; preserve the current native menu until that mapping is established.

## HD inventory and implemented gap

Seven private candidate maps pass the existing source SHA/FNV, compiled target identity, DDS layout, format and full-mip verifier. Existing four-map Volk/glove `priority_material_mode` is unchanged. The base lacked an independent runtime review gate for the already prepared, verified Valkyrie pair.

Added `[general] valkyrie_material_mode` in `texture_override.ini`:

- `0` (default): exactly the existing policy; no new opt-in. Keep legacy `remaster_apply=0` for review.
- `1`: log matching Valkyrie identities, keep existing base/original texture; suppress the legacy remaster route for this pair.
- `2`: opt in only `AR_E99Rifle_alt_d` and `AR_E99Rifle_n`, through the existing bridge/cache/validated loader.

This startup-read setting does not enable assets by default, add another D3D9 hook owner, change device handling or widen the accepted Volk/glove scope. `mode_apply=0` remains the global no-substitution gate. Missing/invalid replacement continues to use existing fallback; MAIN must validate actual intended DDS hash in the deployment manifest.

Verified pair, both 4096x2048 with 13 mip levels:

- `AR_E99Rifle_alt_d.dds`, DXT5 (alpha preserved), SHA256 `A0CDC3DD8528F6B278EF814C77C8F81E7EFA708A12C6D32A30C69B7DC3B59EE9`.
- `AR_E99Rifle_n.dds`, DXT1, SHA256 `54D47F3B7F15EE63410F321B945ACB310DBC95D94346CC006FFD2D7460D06FEE`.

Source diffuse is true 2048x1024 DXT5, SHA256 `93EE1B484C982FA953604A9F020E01C82955C3E7E6D06786C52B4EF7DF6864F4`, FNV64 `16160022635740620080`. Source normal is true 2048x1024 DXT1, SHA256 `05BFB7F5D361E95EEFC3F720BB3731D25F5BE52ECF45BBF519854BEAC9522736`, FNV64 `185541836161454253`. These existing exact targets are reused; no guessed identities.

`stage_first_hires_gate.py --group valkyrie` now creates a fresh private directory with just this pair, its manifest and a default-OFF / physical-PENDING receipt. Default `--group first` still selects only Volk/gloves. Staging verifies all seven inputs before writing, rechecks copied payload hashes, refuses Steam/game roots and never overwrites an output. The local result is `<assets-local>/final-valkyrie4k-review-20261026/`; proprietary DDS files remain ignored/private. The new group totals 16,777,528 file bytes. Approval of visual quality and installation are still pending.

Unfinished assets are explicitly excluded:

- Kasimov normal 4K: exact verified staged candidate, not enabled by this change.
- Kasimov diffuse: available 1024x512 extraction is a lower mip of declared 2048x1024; blocked.
- Centurion diffuse: capture is 1024x1024 DXT5 while declared object is 2048x2048 DXT1; identity/top mip blocked.
- Centurion normal: no verified top-mip/object identity. Upscaling the wrong capture would not repair provenance.

See `docs/textures/hires-first-person-manifest.json` and the existing side-hires recovery report. No new creative asset was generated or silently approved.

## Offline verification

- Python staging/verifier regressions: 6 tests PASS, including exact pair selection, default-OFF receipt, validation-before-write, no overwrite and game-root refusal.
- Private `verify_hires_set.py`: seven maps PASS, including source hashes/FNV and generated compiled target identity.
- Texture C++ policy: 162 material-mode combinations PASS; mode 0 equals previous policy; the independent pair does not affect other targets.
- `spikes/texture_cache/build.ps1`: x86 PE32 `/O2` and static analysis PASS. D3D9 and D3D9Ex pixel tests prove DXT1 RGB, DXT5 alpha/RGB, native/Ex pool selection, reset, invalidation, original-resource lifetime and replacement ownership. Unknown ABI and protected UI/shadow resources fail closed.
- `tools/test-ui-cleanup.ps1`: PASS, including PL/EN prompt scope, both-eye layout at 5264x2684, native selection isolation, wheel state poisoning, front-end fault paths and D3D9/D3D9Ex pixels/state restoration.
- `tools/audit_ui_remaster.py`: source atlas identities PASS; installed identity and physical rendering deliberately NOT RUN.

Generated build logs, JSON receipts, synthetic test binaries and private assets are not committed. No new main `d3d9.cpp` change is required; the changed runtime component is the existing `dinput8.dll` texture bridge. MAIN must build from the integrated source and record its exact final binary hash.

## Grouped MAIN acceptance / stop criteria

First verify source/installed atlas identity, preserved UI flags and accepted features without changing texture quality. Check PL/EN detailed wheel icons, Settings laser/R2, native main/pause navigation, modal text and HUD3D. Native buttons are still the explicitly open remaster item above.

After visual asset approval, review the pair in log-only mode and then one guarded apply session. Require exact pair receipts, correct rifle color/alpha/normal lighting in both eyes, reload and pause/return unchanged. Stop and restore the accepted delta on wrong texture identity, black materials, alpha corruption, different eyes, device/reset errors or material frame-time regression. Existing global texture quality/restart behavior is unchanged. Runtime appearance, asset approval and PSVR2 acceptance remain PENDING; offline pixel tests do not prove them.
