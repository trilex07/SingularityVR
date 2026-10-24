# M02 preflight safety audit (SIDE)

Baseline `9ed671d`; Steam and accepted installation are out of scope.

## Observed source contracts and falsifiers

- P1: `remasterSettingsReady` gates only the legacy selected-row fill and native footer `ClearRects`. Native title and settings rows are still submitted. However, the atlas footer is 900x32 and is scaled to the panel's much narrower footer rectangle; the accepted native footer is suppressed even though its readability is not established. Hypothesis: this can remove the only reliably legible navigation hint. Falsifier: pixel/readability test demonstrating the scaled footer is at least as legible in both eyes. Smallest safe correction: retain the native footer and omit the baked footer for now.
- P2: `DrawUiRemasterWheelEye` draws chassis/icons directly to the backbuffer, then returns false on a later invalid rectangle or failed draw. The caller then draws the accepted wheel in the same eye. This is a proven non-atomic partial-overlay path. Hypothesis: an offscreen staging target can make all source draws fail without changing either eye, then one two-eye composite call commits the wheel. Falsifier: failure-injection test in which a source draw fails but any new-wheel pixel reaches the backbuffer, or the native fallback is missing.
- P0/P1/P2: all flags read default 0 and P1 depends on P0's neutral atlas; P2 has a separate gate. Verify each of the eight switch combinations, missing assets, Reset, and restored device state. Offline tests are not PSVR2 acceptance.

## Corrective changes

- P1 retains the native `PanelPageFooter` for both eyes. Atlas header, tab, focus and Sense icon remain decorative; native title and rows are drawn after them. Settings input and hit rectangles are unchanged.
- P2 stages the entire wheel in a reused A8R8G8B8 D3DPOOL_DEFAULT render target. All atlas draws, including text, finish there before a single four-triangle call composites both eyes. A staging error falls back to the accepted wheel with no remaster pixels on the backbuffer. A composite error withholds the fallback for that frame because partial GPU submission cannot be ruled out. The render target and viewport are restored before fallback, and the outer state block restores textures, blending, shaders and other D3D9 state. Unsupported render-target creation is latched until Reset to avoid frame-by-frame retries.
- P0/P1/P2 gates remain separate. `SettingsEnabled` requires P0 and P1; `WheelEnabled` requires only P2. Reset releases the staging target. The accepted wheel path is unchanged when P2 is OFF.

## Offline verification

- CPU geometry and eight switch combinations at 5264x2684: PASS.
- D3D9 and D3D9Ex HAL fixture: PASS, including an injected post-draw staging abort, equal two-eye composite pixels, and render-target/viewport/state restoration.
- Settings panel hit rows, pause settings, wheel layout and catalog: PASS.
- Dual TMD, glove, WeaponLocker and OLED regressions: PASS.
- x86 `/O2` PE32 build: PASS. Steam and PSVR2: NOT RUN.
