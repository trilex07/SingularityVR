# Gameplay ShowInformation location text — source-side safe area

## Original producer, verified offline

The privately decoded original `RvGame.decompressed.xxx` contains one `HUD` GFx movie: 187541 bytes, FNV64 `21ACFC3AD63D0F40`. Its AVM1 root `ShowInformation` function calls `locationTxt.show`. `initSinglePlayer` constructs `hudShared.components.LocationTextDisplay` as `locationTxt`. `TextDisplay` attaches its named holder under `txtMsgContainer`; `LocationTextDisplay.NAME` is `LocationTextDisplay`. The exact holder is therefore `_root.txtMsgContainer.LocationTextDisplay`. Export 414 has three named text fields (`msgTxt0`, `msgTxt1`, `msgTxt2`, characters 411–413), with animation on the holder. Its `positionDisplay` explicitly requests `ALIGN_BOTTOM` and `ALIGN_LEFT`, then adds 100 px to `_y`. This is the source of the corner placement. `ObjectiveTextDisplay` is export 410 and is a different component. NPC name has its own `npcTextField.nameTxt` path.

The original export-414 shape 218 has bounds `[-5120,0,-640,640]` twips and an authored matrix at `(4940,820)` twips with Y scale 4. That gives local pixel bounds x=-9..247, y=-87..169. A 256 px holder at origin `(521,439)` places that content at x=512..768 and y=352..608 on the 1280x720 source stage. Both 2632x2684 SBS eyes receive the same normalized source geometry. The focused CPU test checks those bounds for both eyes at 5264x2684.

## Implementation

`[Render] VrGameplayTextSafeArea=1` is default OFF. On the existing engine input thread, only during VR gameplay, the patch resolves the current local controller → myHUD → mGFxHUD. It requires the exact original movie length/FNV and the already audited numeric getter/setter vtable fingerprints. It reads the dimensions and position of only `_root.txtMsgContainer.LocationTextDisplay`; absent/unknown clips and dimensions outside the narrow component range are refused. It sets that holder's `_x/_y` to lower centre and reads back the result. The original three text fields, strings, Polish localization, type-in, visibility and fade remain in GFx. It neither edits packages nor copies text to another renderer.

This includes location text and other information passed through `ShowInformation`. It excludes objective text, NPC names, subtitles, pickup prompts on other components, HP/ammo, crosshair, pause/main menus, VR Settings and M02/M09.

The prior `[Diagnostic] GameplayTextProbe` stays independent and default OFF. It is not needed for this targeted candidate.

## Validation and MAIN handoff

- Private original-asset identity test: PASS, including export IDs, three field owners, authored shape matrix and movie FNV.
- x86 placement policy test: PASS, including both-eye geometry and invalid/absent clip refusal.
- x86 PE32 `/O2` build: PASS. DLL SHA256: `B839ADD4C9A794E6B6948E73B2085585B0FEDDD618173682239D23CB322F0E5E`.
- D3D9/D3D9Ex and TMD/weapon/OLED/WeaponLocker regressions: PASS. No direct runtime GFx pixel proof.

MAIN can review this branch after its combined M02/M09 acceptance. Keep Steam's accepted DLL/INI backed up. Enable only `VrGameplayTextSafeArea=1` and enter a known location with a `ShowInformation` label; verify readable lower-centre text in both eyes, its original fade/timing, and unchanged objectives, subtitles and ammo. Close normally and archive the original `gameplay location text safe area` readback receipt and screenshot. If no receipt appears, inspect the exact component path/lifecycle before changing draw coordinates. SIDE did not deploy or claim physical PASS.
