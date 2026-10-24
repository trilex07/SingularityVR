# M02 P2 detailed weapon icons — asset-only handoff

Branch: feature/side-m02-detailed-weapon-icons
Source baseline: e379b7afbcc40eb6e2bf6331b49feee833330820 (combined M02 P2 + M09 candidate)

## Scope and provenance

- Four neutral weapon sprites changed: Centurion (5799), Valkyrie (11387), Volk/Spikeshot (8297), Kasimov (11389).
- Designs used side views of locally extracted original game meshes for Centurion, Valkyrie and Kasimov, plus the game's Volk screenshot and prior icon. The committed masters are new illustrations; no extracted proprietary mesh or texture is committed.
- Only the neutral atlas needs installation for a physical test. No DLL, INI, PL/EN atlas, input, wheel geometry or M09 change is required.
- The accepted wheel stretches each rectangular source icon into a square quad. The updated sprites use a shorter content band to keep the final weapon proportions recognizable without changing that renderer.

## Exact asset delta

Old neutral atlas SHA256: E4DC0E7D209BDDF23816F101177C8A8B02AA24ED766C55C446877956CAC4F23B
New neutral atlas SHA256: 3EF1782696B4A5E1E9D93F409C5B0F6FF014B78CC42FAB06EC29110661981818

The complete per-sprite source and output hashes are in assets/processed/ui_remaster/atlas/detailed_weapon_icons_manifest.json. The original ui_atlas_neutral.json and every pixel outside the four original rectangles are unchanged. The historical docs/integration/MASTER_INTEGRATION_MANIFEST.json remains pinned to the pre-polish baseline; use the new delta manifest for this optional replacement.

Reproduce with python asset-tools/build_detailed_weapon_icons.py. The complete V2.1 generator also reads the new master PNGs rather than reverting to its procedural placeholders.

## Offline verification

- python asset-tools/test_detailed_weapon_icons.py: PASS; four distinct sprites, unchanged UVs and dimensions, exact source/atlas pixels, no modified non-icon pixel, unchanged PL/EN atlases, transparent margins.
- python asset-tools/test_ui_atlas_qa.py: PASS; 163 sprite rects, UV bounds, alpha and gutters, localization.
- Existing /O2 fixtures: test-ui-remaster-runtime PASS at 5264x2684; test-ui-remaster-d3d9 PASS for D3D9 and D3D9Ex, two eyes and state restoration; test-ui-remaster-switch-combos PASS; test-tmd-haptics PASS.
- Visual checks: docs/previews/ui_remaster/detailed_weapon_icons_compare.png shows old/new in idle and selected states at a 144-pixel square icon quad; detailed_weapon_wheel_preview.png shows all four together.

No DLL rebuild was needed because no runtime source changed. No Steam or PSVR2 validation was performed on the new artwork. Status: READY_FOR_PHYSICAL_ACCEPTANCE.

## MAIN acceptance and rollback

With the game closed, back up and hash the installed neutral atlas. Use the guarded asset deployment procedure to replace only Binaries/vr-assets/ui/remaster/ui_atlas_neutral.png with the new hashed file. In PSVR2, open the wheel with all four weapons acquired, check each icon in idle and selected states in both eyes, and verify that the real ammo text, selection and M09 feedback still behave as accepted. On a visual failure, restore the exact backed-up neutral atlas and verify its hash. Do not replace PL/EN atlases or the DLL for this cosmetic test.
