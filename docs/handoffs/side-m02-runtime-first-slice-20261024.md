# SIDE M02 first runtime slice — MAIN handoff

This branch adds a single decorative atlas-backed plate behind the existing VR Settings contents. `[Render] UiRemasterRuntime=0` is the default and exact rollback switch; `1` enables the slice. No Steam files were changed by SIDE.

## Runtime contract

- Only the neutral atlas is used in this first slice. Source asset: `assets/processed/ui_remaster/atlas/ui_atlas_neutral.png`; copy it as `Binaries/vr-assets/ui/remaster/ui_atlas_neutral.png` for a controlled MAIN candidate. Expected SHA256: `E4DC0E7D209BDDF23816F101177C8A8B02AA24ED766C55C446877956CAC4F23B`.
- The atlas is decoded and uploaded only if the flag is enabled and VR Settings draws. D3D9Ex-compatible dynamic DEFAULT-pool texture; failed file/decode/texture creation leaves the accepted panel in place. Reset releases the texture and state block.
- The plate rectangle comes from the current `settingspanel::Layout`; it is non-interactive. Native GFx, text, laser target and R2 hit-testing remain unchanged. The original panel background still renders when enabled, behind the atlas decoration.
- `D3DSBT_ALL` captures/restores the device pipeline around the two eye draws. Render target and viewport stay under the existing `DrawVrPanel` save/restore. Bounded `M02 UI:` log receipts show asset readiness, eye draw count and state-block restore result.

## Evidence and scope

- CPU geometry fixture: mono and stereo bounds at 5264x2684, symmetric eyes, atlas UV bounds.
- Off-game D3D9 and D3D9Ex HAL fixture: both eyes submit; equal nonblack pixel counts and restored texture/blend state. A HAL creation failure in a sandbox means BLOCKED, not a pixel PASS; run this fixture on a desktop HAL.
- Existing dual-weapon/TMD/reload/WeaponLocker suite, Settings panel test, OLED test and x86 `/O2` build pass. The broader `tools/tests/test-menu-polish.ps1` reaches a pre-existing baseline assertion: `FrontMenuScalePct=72` is absent from this branch's tracked INI, unchanged by M02; do not label that suite PASS without a separate baseline repair. See final commit report for exact DLL SHA.
- No headset rendering is claimed. PL/EN atlas, wheel, icons, prompts, weapon HUD, audio and 3D HUD are not integrated by this slice.

## MAIN controlled visual gate

Back up and hash accepted DLL/INI/assets first. Stage only the reviewed DLL, neutral atlas and `UiRemasterRuntime=1`; keep other accepted settings unchanged. Open VR Settings in PSVR2: require the decorative plate in both eyes, no clipped text, native controls and laser/R2 still aligned, and no visual/state leak after closing/reopening or graphics Reset. Archive full log and screenshots. On failure set `UiRemasterRuntime=0` and verify the accepted visual baseline; do not expand M02 until this gate passes.
