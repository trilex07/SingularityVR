# Cinema frame disparity follow-up

## Physical evidence

C84569BF: user confirms weapon attachment is correct. Cinema FAIL: right-eye edge ends slightly earlier than left-eye edge. Retain accepted gun behavior; no weapon changes in this follow-up.

Actual current-session log copied privately to build/cinema-eye-followup/evidence/session.log. Lines 9985-9989 prove both quads have equal 2.6062 x 1.3290 metre dimensions, equal pose, equal 2820x2876 rects and shared swapchain. Native renderer logs 0.064 m eye separation. Thus unequal submitted dimensions are disproven for these receipts. The previous common-FOV fit did not remove binocular frame disparity.

## Falsifiable correction

Equal world-space quad poses produce different eye-relative edges because eyes are separated. A 64 mm / 2 m plane yields 0.032 tangent difference at a shared edge. The geometric fixture reproduces this exactly. This mechanism is proven mathematically and matches the description; attribution of the user's perceived discrepancy remains a hypothesis until PSVR2 confirmation.

Translate each eye-only quad by eye.position - centre(eyes.position). This makes the frame zero-disparity for parallel eye orientations while retaining independently rendered scene-eye content. Keep width, height, aspect, world-anchor orientation, shared cyclopean screen location, source rectangles and native stereo camera unchanged. Recompute the small eye translation using the captured frame's poses, not live later poses. Invalid tracking/IPD retains the existing common-pose fallback. Mono Bink, gameplay, hands, gun and input remain untouched.

This intentionally changes the frame depth convention: the border no longer has the physical screen-plane binocular disparity. The scene itself remains stereo. No per-frame readback, resource allocation, GPU synchronization or new hooks.

Falsifier: if the edge discrepancy remains with translated poses, inspect content/matte bounds and actual runtime eye orientation; do not keep changing scale. Canted eye orientations or compositor/lens FOV limits can still affect apparent extent; do not claim physical PASS from geometry.

## Validation and deployment

- x86 /O2 geometry fixture PASS: old discrepancy reproduced, all four corners identical in eye space across twelve head/turn positions, cyclopean anchor fixed, orientation retained, invalid refusal.
- Stereo D3D9Ex pixel/state, gameplay and optimized build results recorded with final artifact identity below.
- Active game is not modified or restarted. New candidate deployment requires game closed. No automatic rollback timer.
- One targeted PSVR2 check: same scene, compare left/right border positions and full image, then head turn and return to gameplay. Gun attachment already confirmed and unchanged.

## Final offline candidate

- Source 0a790b3a; branch fix/cinema-zero-disparity-frame.
- Optimized x86 PE32 DLL EBA33BEDA274C0DC11890FF62235AE3E74B3497B7EF3DD54D282AD3DEF79A24F.
- x86/x64 gaze, cinema, damage haptics and mutant fixtures PASS; D3D9Ex pixel/state PASS. Legacy D3D9 HAL NOT RUN (host device rejected).
- Actual compiled factory/FName ABI PASS, literal [eax+20Ch] preserved.
- Private artifacts, exact change plan and accepted profile prepared in build/cinema-eye-followup. Steam not modified during active session; physical check PENDING.
