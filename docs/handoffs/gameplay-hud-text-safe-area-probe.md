# Gameplay HUD text safe area — source identity gate

## Observed issue and evidence

The wearer reports location names and contextual gameplay labels at the lower-left edge, often outside comfortable PSVR2 view. The accepted renderer has a per-eye `HudStereoPair` route for scene-target orthographic draws, a separate ordinary `StereoPair` route, and offscreen GFx targets. `RvGameSharedHUD.mGFxHUD` is reflected and its HUD movie is fingerprinted for the NPC-name scale path, but no evidence identifies the location label's GFx field, draw batch, texture, or route. Historical note/popup measurements cannot establish this identity: different UI producers crossed different draw paths and attempts to move ambient HUD draws broke scene geometry. The pause menu is already accepted and is outside this work.

## Hypothesis and falsifier

A native orthographic gameplay label may arrive at a lower-left c5-c8 clip translation. If so, its draw signature should appear while the label is visible, with a route and texture identity that can be compared against the same scene before it appears. If no matching new draw is recorded, or if the same signature belongs to HP/ammo/subtitles, this hypothesis is false; do not shift that batch. A GFx offscreen producer needs a different source-side correction.

## SIDE candidate

`[Diagnostic] GameplayTextProbe=1` activates a read-only, sampled census of lower-left orthographic user-pointer draws in gameplay. It records the original clip translation, stride, primitive count, bound texture identity, scene/offscreen target and planned stereo route. It samples one in four eligible draws, caps matrix queries at 4096 and unique log receipts at 96. No render state is changed. Default OFF leaves the accepted image byte-for-byte unchanged. The requested `[Render] VrGameplayTextSafeArea` behavior is deliberately not enabled until the actual producer is identified; this branch is diagnostic, not a claimed visual fix.

## One bounded MAIN evidence capture

After M02 testing, MAIN can build this branch, preserve the accepted installation and enable only `GameplayTextProbe=1`. Load a save immediately before a known area-name popup. Stand still for 2 seconds, step into the area, observe the text for 3 seconds, then close normally. Preserve the complete `view_matrix.log` and a right-eye screenshot before another launch. Compare `gameplay-text candidate` timestamps and route/texture/clip tuples against the stable two-second baseline. A source-side GFx field may be preferable if the same movie owns the text. Do not enable a placement fix based solely on lower-left coordinates.

The expected next result is a specific producer or a precise missing observation. SIDE has not run Steam or PSVR2 and claims no visual PASS.

## Offline verification and limits

- The focused x86 probe test passed scope, orthographic-matrix, route-identity and 96-entry budget checks.
- The x86 PE32 `/O2` build passed. Built DLL SHA256: `7CF2BEEC99AA5C42A6B4E357C1655F3708A75741BC5A67EDED422F2F641D4B29`.
- TMD runtime regressions passed on MAIN's local D3D9 and D3D9Ex fixtures, including two-eye SVTG draws, pixel checks and index-buffer restoration. Those checks are non-regression evidence, not proof of gameplay-label identity.
- `tools/tests/test-menu-polish.ps1` passed its CPU menu checks, then failed on the baseline `FrontMenuScalePct=72` INI expectation. This branch does not edit the INI or pause/main menu.
- The probe covers orthographic user-pointer draws. If no area-name-correlated signature appears, the text may use GFx offscreen composition or another D3D draw API; the absence must not be interpreted as a visual fix or as proof the text does not render.
