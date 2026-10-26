# Final visual integration handoff â€” 2026-10-26

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

## Original HD inventory and first implemented gap (superseded below)

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

Historical exclusions from the first pass, resolved by the recovery continuation below:

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

Clean-source bridge receipt: implementation commit `7548b1196047219ca2e51cad5616fd1650b0b6c6`, tracked diff empty, x86 PE32 `/O2`, SHA256 `1C4EFA71112CADEDA023FB35FE5091F55AE11F8892754E189ED5EFC106AC2027`. Local artifact and input-hash receipt: `build/final-visual/texture-cache/dinput8.dll` and `dinput8.build.json`. This documentation follow-up does not change those compiled inputs.

## Grouped MAIN acceptance / stop criteria

First verify source/installed atlas identity, preserved UI flags and accepted features without changing texture quality. Check PL/EN detailed wheel icons, Settings laser/R2, native main/pause navigation, modal text and HUD3D. Native buttons are still the explicitly open remaster item above.

After visual asset approval, review the pair in log-only mode and then one guarded apply session. Require exact pair receipts, correct rifle color/alpha/normal lighting in both eyes, reload and pause/return unchanged. Stop and restore the accepted delta on wrong texture identity, black materials, alpha corruption, different eyes, device/reset errors or material frame-time regression. Existing global texture quality/restart behavior is unchanged. Runtime appearance, asset approval and PSVR2 acceptance remain PENDING; offline pixel tests do not prove them.


## HD completion continuation — exact source recovery

This section supersedes the earlier Centurion/Kasimov blockers. No UI source was changed in the continuation; MAIN owns the native button producer work.

**Centurion identity is now proved.** Original `ch_inview_alt_pistol_mesh` in private `wp_SP_ARPistol_SF` references `AR_Pistol`, whose exported native material binds `Diffuse=AR_Pistol_d`, `Normal=AR_Pistol_n`. Both are true cooked mip0 1024x1024. The old source-recovery report inspected unrelated `wp_handgun` textures declaring 2048x2048. It was the wrong source family. The recovered original compressed pixel hashes are exactly the already captured runtime aliases `handgun_d` / `handgun_n`; these aliases and their target/group behavior are deliberately retained, without ambiguous duplicate targets.

- `AR_Pistol_d`: DXT5, source SHA `0C958ED11C3311DDBB517A8893EC7E372D0811A58582220F04A6EFD30A0D9A39`, FNV64 `5208987696962520514`, cache offset `866385920`, raw SHA `E05B476436A3B64A6CC800657672454690A426FC2DAA0EA3C538B1BDA4BE4297`.
- `AR_Pistol_n`: DXT1, source SHA `AE206388BB839175C3D3D0DB453A7B7A0F0505B6DB76FC3DC46D950EC23AE2D8`, FNV64 `6877428834008494221`, raw SHA `18369932A7A8EB898C4B28142267845A5F20D9B351865D15C56079587A0C62BA`.
- Logical SP package SHA `0534C5C9A5878D280294168BD12711B99CB3948503D4ED53608A0316479D1B18`.
- New runtime files `handgun_d.dds` / `handgun_n.dds`: both 2048x2048, 12 mips, SHA `3590579EDF7F67ED49675B0104798EA6D66818E9B2663C5BCCCFFE3DFE618E9C` / `ED70B5EB67E85BCCA99B305865C574F7023DA622D0EC8E9A41DC1F352D67E255`.

**Kasimov diffuse first-present mip is now proved.** Both decompressed SP and MP packages serialize nominal 2048x1024 mip0 as UNUSED: bulk flags `0x21`, element count 0, stored size/offset -1. This is not a decoder failure or a valid missing cache offset. The first present mip is index 1, 1024x512 DXT5, external LZO at `0x2E310000` (`774963200`), compressed size `451385`, raw size `524288`. Independently decoding that exact cache range produces bytes identical to the UModel DDS payload. Source SHA `AC7D2276FFF2042FAC8EA210519115CF712719F83162E914557EC7B83B7E936B`, FNV64 `14906919060078924434`, raw SHA `73F2E3483B53914A06A068A7ADC22C6A22309D242438EB7D50AF7EB279BFE240`. Logical SP package SHA `6C1F0910BF932EB62E5B67A1B9DE5B370E0DA65168F97D71495CFF598FF2EEBC`.

As additional research, all 66 valid 2MiB cache chunks were decoded and compared with the known diffuse thumbnail; best correlation was 0.189, providing no support for an orphaned full diffuse. That search is supporting evidence, not proof of the identity of any unreferenced chunk. No unreferenced data was admitted.

`ar_sniper_rifle_d.dds` is a real 4096x2048, 13-mip DXT5 candidate, SHA `61E4CC32ED2DED5E369076C5E4CCF4F099D09EA52F63DEF76E9669B4ED40B7A8`. It is **4x interpolation of the highest present 1024x512 source**, not recovered 2048x1024 detail. The existing verified normal stays 4096x2048 / DXT1 / 13 mips, SHA `37EA2C1A3C368E1A2B7428490D5A36D9673AB0244DFC5AF013AC604C17DAED77`. `ch_inview_sniper_mesh -> AR_SniperRifle -> ar_sniper_rifle_d/n` was verified through the original exported mesh/material chain. The lens, specular/cubemap and animation assets remain original.

All exact bulk receipts reference private `Textures_P0.tfc` SHA `6BE9134DF84D23B49089DA39B05594EE813B350949318A54684273104D865EE0`. Only hashes/scripts are committed, never game pixels/packages. Source DDS is copied only to new private source names; no prior source is overwritten.

### Reproduction and usable private stage

1. Existing `tools/uedecompress/unpack_chunks.cpp` correctly decompresses SP/MP packages into private logical `.u` files. The older whole-package-only decoder cannot read these internally compressed packages. Build the existing chunk tool offline with the repository's MSVC environment; output/package bytes stay outside Git.
2. Export exact objects using existing UEViewer: `-export -dds -path=<assets-local>/cooked -out=<private-recovery>/exports wp_SP_ARPistol_SF AR_Pistol_d` and `_n`. UModel reports mip0 for both. Existing Kasimov diffuse export is mip1 and must retain this label.
3. Build `tools/textures/read_tfc_bulk.cpp` with MSVC `/O2 /EHsc` as a standalone **offline GPL decoder tool**, not part of the shipped DLL. It reuses the existing decoder, validates full chunk/block lengths and refuses an existing output.
4. Run `tools/textures/audit_cooked_texture.py --package <private-logical-package> --name <original-name> --source <original-dds> --tfc <private-cache> --decoder <offline-tool.exe> --output <new-private-audit.json>`. Source descriptor, first-present mip, compressed range and DDS bytes are checked independently.
5. `python tools/textures/prepare_remaining_hires.py --assets-root <assets-local> --output <new-private-output>` verifies pinned source and cache receipts before producing the three missing DDS. It uses the existing independent-alpha interpolation and normalized-normal pipeline, full chains, unchanged DXT formats, no generated scratches or model changes. Previews label the actual cooked mip and interpolation factor.
6. `python tools/textures/sync_hires_targets.py --write` adds only missing exact targets, refusing changed identity or duplicate aliases. Only Kasimov diffuse is new; the Centurion and normal targets were already captured.
7. Run the existing `verify_hires_set.py --stage <assets-local>/material-remaster-hires/ready --originals <assets-local>/textures/original`. All **10** inputs pass exact source SHA/FNV, runtime registry layout, replacement hash and complete mip-byte checks.
8. `python tools/textures/stage_first_hires_gate.py --assets-root <assets-local> --name <new-private-stage-name> --group complete` produces the complete 10-map stage, rechecking payload hashes after validation, never overwriting, never targeting Steam/game paths.

Ready local stage: `C:/Users/Trilex/vr-mods/SingularityVR-assets-local/final-complete-weapon-hd-review-20261026/` — **55,925,552 DDS bytes**. It contains Valkyrie4K, Kasimov4K, Centurion2K, Volk2K and glove2K pairs, `manifest.json`, and `review.json` with actual independent INI keys and physical PENDING. Existing Valkyrie/Volk/glove/normal payload hashes are unchanged. Private recovery receipts and previews are under `material-remaster-hires/recovery-20261026/` and `material-remaster-hires/centurion-kasimov-complete-20261026-v2/`.

### Runtime gate / preservation contract

Added `[general] centurion_material_mode` and `kasimov_material_mode`, both default **0**, with the same existing modes 0/1/2 as the Valkyrie gate. No existing `priority_material_mode` membership, device hook, owner, cache lifetime, image loader, NIS/OLED, main renderer, HUD3D, glove pose, localization or UI source is changed. Each explicit mode2 admits only its exact two named targets. Mode1 logs identity and does not select its remaster. `mode_apply=0` remains global no-substitution. Changing these startup-read gates requires restarting the mod, matching the existing texture policy.

With every new gate at 0, existing legacy selection is unchanged. In particular, the newly registered Kasimov diffuse target is ignored completely at mode0, preventing baseline base-directory replacement just because a file exists. Kasimov normal keeps its old existing policy. Existing Centurion aliases retain their historical `hands` texture group (so review requires that group enabled); all other weapons remain in `weapons`. No capture alias is renamed and no accepted file is overwritten.

MAIN needs to copy the reviewed files through its normal backed-up deployment, point the existing `remasterdir` at that private reviewed payload in the candidate, retain `bridge_owned=1` under the accepted single-owner bridge, and deliberately choose the individual modes. Source presence does not establish that the installation contains these files. Keep general legacy `remaster_apply=0` during this narrow review. The newly proved Kasimov diffuse FNV must actually match a bound resource before substitution; no layout/hash fallback is permitted.

### Continuation verification and final acceptance

- Python texture suites: **14 PASS**, including stripped-mip versus first-present parsing, malformed descriptors/bounds, FName property contract, target idempotence/collision refusal, independent-alpha 4x interpolation, 10-map staging, no overwrite and all independent mode receipts.
- C++ actual material policy: **1944 combinations PASS**, all four review gates, legacy defaults and exact identity scopes. New diffuse cannot activate with Kasimov mode0.
- Exact private DDS/cache audit: 3 recovered maps PASS, full raw payload byte equality. Full public/private verifier: **10 maps PASS**. Repeated deterministic generation produced identical three DDS SHA256s.
- Build/static analysis and D3D9/D3D9Ex GPU tests: PASS, native RGB+alpha pixels, native/Ex pool, Reset/ResetEx, invalidation, resource-owned replacement lifetime, protected UI/shadow sources and unknown-ABI refusal.
- No full main `d3d9.dll` source/input change. The runtime artifact is the existing bridge `dinput8.dll`; a clean-source receipt is recorded in the delivery follow-up.

**GO for MAIN review and grouped candidate integration; visual approval and physical PASS remain PENDING.** One session: inspect all four acquired ordinary weapons and left/right glove under the same light, rotate wrists/weapons, reload Valkyrie and Centurion, view Kasimov scope, then pause/return. Require expected exact BRIDGE MATCH and SUBSTITUTE receipts, the correct material/alpha/normal lighting in both eyes, no black/transparent surfaces, no new shimmer or reset error, and acceptable measured frame times. Stop/restore only this texture delta on wrong source identity, missing pair, alpha/normal corruption, stereo divergence or performance regression. Offline interpolation and pixel tests cannot establish artistic approval, perceived detail or headset performance.

Clean continuation build receipt: source commit `cbda85b9363112bc29a9bb45a13300503cb413d5`, tracked diff empty, x86 PE32 `/O2`, SHA256 `E5DD01D13A9102A1CEF2E70C95FB1796E191277FB5086D697D0EE81E699394FA`. Artifact/input-hash JSON: `build/final-visual/texture-cache-hd-final/dinput8.dll` and `dinput8.build.json`. This documentation-only follow-up does not change compiled inputs. MAIN must independently rebuild and hash its combined candidate.
