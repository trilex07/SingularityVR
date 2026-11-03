# Cinema mono bootstrap and missing-caption correction — 2026-11-03

## Actual evidence

User corrects the report: captions visible in RIGHT eye, absent in LEFT. Installed 59171C58 verified by hash. One-shot audit DID execute: frame1533, source5640x2876, capture_2026-11-03_11-38-05.bmp, xrEndFrame=0. Private raw evidence archived under build/cinema-orientation-audit/session-1138/. No headset confirmation UI was expected.

Both final quads are 3.1274493x1.5932987 with distinct full-eye image rectangles. Edge angles L/R/D/U approximately -66.437,+60.127,+23.195,+70.309 degrees. Actual left FOV is -61.5,+43.446 and right -43.446,+61.5, vertical +/-53.040. The submitted screen is clipped differently by actual asymmetric view frusta. Equal edge rays alone did not ensure equal visible bounds. Captured source contains both eye images. Pause-time 11-38-43 capture also contains the native production credit only on the RIGHT half, independently corroborating a source-side text defect; it is not proof of every subtitle producer.

## First source contract violations and minimal changes

1. Session log explicitly shows a mono LOCAL quad anchor BEFORE the first independent stereo receipt. Anchor::LatchStereo previously returned immediately on any valid anchor, even one created by mono Latch. Consequently the prior binocular fit was never run on that transition. Add stereoFitted state: mono is not a stereo fit; first successful stereo latch validates both frusta and marks ownership, subsequent stereo frames preserve anchoring. Reset clears both. Bounded receipt reports actual fit execution.
2. Hook_SetVSConstF returned at CutsceneNativeCameraArmed BEFORE the established targeted GFx c5-c8 upload classifier. This leaves the HUD marker unset during native cinema; draw hooks therefore skip HudStereoPair/CanvasEyes, whose existing two-eye dispatch never executes (zero cinema-caption receipts in this session). Move the EXISTING classifier ahead of that native return, after the unchanged offscreen/shadow exclusions. No new shader register, camera transform or classification algorithm. Original constants are forwarded unchanged. Actual subtitle rendering still requires runtime confirmation: pixel fixtures alone do not prove that every native caption uses this producer.

No weapon, tracking, TMD, HUD, input or native aim change. No generic geometry suppression or screen-space text masking. A world-anchored screen can still leave the FOV when the head deliberately turns away; initial fitted bounds are not a claim of perpetual visibility under arbitrary head motion.

## Validation

- Executable fixture compiles the ACTUAL hook prefix from d3d9.cpp through its native-camera early return, with stubbed D3D forwarding only. Old47d4274b FAIL: HUD upload witness remains unset. New x86/x64 /O2 PASS; original data pointer forwarded once, shadow/offscreen/bulk/self writes excluded, gameplay path preserved.
- Mono->stereo bootstrap fixture reproduces bypass; corrected stereo fit is smaller with asymmetric frusta, sticky on subsequent stereo frames, reset on exit. x86/x64 PASS.
- Existing actual D3D9Ex caption pixels, stereo scene/corners and restoration PASS. Legacy D3D9 HAL NOT RUN (host8876086C). Gaze, damage and mutant fixtures PASS.
- Build/compiled ABI identity recorded after build. Steam installation remains diagnostic59171C58 until an explicit guarded deployment; no claim of physical PASS.

## One acceptance gate

Same cinema: captions readable and identical in both eyes, equal borders while looking at screen centre; turn away/back and pause/resume. Runtime must record successful stereo fit and caption dispatch, plus accepted xrEndFrame. Preserve accepted right-gun attachment. No further speculative placement changes if captions bypass the measured producer: capture the actual submitting shader/draw instead.

## Built and deployed candidate

Source c6ee44eddfadd4116c2d10f045d8fb523a90a235; optimized x86 PE32 /O2 DLL SHA256 06633749B829E9859B65300F476DE423B4122582DA77A8B3F559274FB36F546F. Actual compiled factory ABI regression PASS (literal [eax+20Ch]). MAIN deployed DLL only after process exit and elevated writer inventory; backup deployment/install-backups/20261103-114647, report deployment/install-reports/20261103-114647.json. Full protected-profile Post verification PASS. INI preserved byte-for-byte SHA A9F4C8932E2C49DBC524B7612C43A4F03A914B00F3900C925A320F6D8202BEA7; one-shot audit remains enabled for next run. No launch/timer initiated. Physical cinema and caption outcome PENDING; no merge to main.
