# Native stereo cinema: gameplay-lock lifetime correction

Branch: `fix/gaze-newgame-bink-crash` (also contains the stereo follow-up).

## Actual evidence

The user repeated New Game with candidate 623D3EFD. Intro played; the helicopter was physically reported mono. The external read-only exception capture recorded no exception. The earlier Bink integer-overflow crash remains UNRESOLVED/NOT REPRODUCED, not fixed by this change. No Bink codec or audio-track behavior is changed.

The archived session log records gameplay c0 ROW acquisition, then `lock ... stopped verifying for 61 frames - DROPPING`, including before the helicopter transition (frame 5726). Mono composition continues throughout that cutscene. Source confirms `CutsceneNativeCameraArmed()` returns before gameplay matrix verification/scanning, while `StereoNativeCinema()` required that expiring gameplay lock. This is the first confirmed stereo eligibility violation.

The initially suspected Present token ordering is NOT the cause: `UpdateFromHeadset()` and capture occur before `g_armPaletteFrameToken` advances. No token-order change was made.

## Minimal correction

Retain only the measured ROW/COL storage convention when the existing gameplay scan acquires a lock or a native upload verifies it. Cinema uses that convention independently of gameplay lock lifetime. Each cinema draw still requires scene target, non-shadow shader, exact CTAB ViewProjection, complete native constants, current matrix shape, tracked eye separation, and successful two-eye dispatch/state restoration. No cached camera matrix or stale controller pose is reused.

The storage convention is not a D3D DEFAULT-pool resource, so device Reset clears existing native constants/frame receipts as before but does not discard the measured renderer convention. Cold startup without any valid convention still falls back safely. Gameplay camera, hand frame tokens, Bink, gaze and haptics are unchanged.

A bounded 24-transition completed-frame receipt distinguishes stereo/mono through startup attract scenes and the helicopter; no GPU readback or synchronization was added.

## Validation

- Optimized x86 and x64 convention lifetime regression: PASS, including 180 native-camera frames, absent/expired gameplay verification, invalid current matrix, changed measured convention and explicit reset of the helper.
- D3D9Ex pixel fixture: PASS, distinct left/right centroids 89.63 / 70.37, 138 pixels per eye; restored constants/scissor on successful and failed dispatch.
- Legacy D3D9 HAL: NOT RUN, host rejects CreateDevice with 0x8876086C.
- Gaze ABI/freshness/wheel/fovea fallback and damage haptics: PASS.
- Real helicopter stereo and crash recurrence: PENDING. Offline pixel tests are not PSVR2 proof.

## One physical acceptance

New Game through Steam. Observe the native helicopter scene for over ten seconds: a stable cinema screen with actual scene depth, correct eye order, no head-driven authored-camera motion or duplicated hands. The prerecorded Bink intro remains mono by design. Confirm normal gameplay returns afterward. If it exits, preserve the log/dump; do not call the earlier crash resolved based on a successful repeat.
