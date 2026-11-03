# Current-wrist attachment and common cinema fit

## Accepted versus proposed

The user accepted 97F3D3FF as nearly correct and authorized promotion. Remote main was fast-forwarded to 009d2d12. Residual drift and cinema edge symmetry are not physically accepted. The installed 97F3D3FF DLL and existing INI remain the baseline.

## Evidence and minimal change

The gun previously extrapolated a normal wrist through the raw controller transform. The current independent glove instead consumes the original current-frame wrist palette, current LocalToWorld and final BuildGunC ride. These are different transformations when animation/camera-local ownership changes. The frame-identity-checked original wrist capture now publishes sourceJoint * local * finalRide to the existing gun correction. The accepted gunFromWrist calibration, bone selection, displacement/freshness guards and fallback remain unchanged. The shared single-hand policy selects the same target as the actual glove; no new offsets, controller sampling, aim, support or input changes.

Falsifier: a runtime receipt with exactCurrentWrist=1 followed by persistent gun/glove relative drift would show that palette ownership/capture ordering still differs in that draw. Receipt is bounded to the existing first six corrections. Offline reconstruction uses the actual LocalDeltaPair/TransformBone path, then checks fixed gunFromWrist under twelve varying camera-local/turn/hand poses; it also reproduces divergence of the old extrapolation.

Accepted-session cinema receipts (private build/tmd-rigid-cinema/evidence/accepted-97f3-session.log, lines 3799-3800) already show identical physical quad pose/size and 2820x2876 source rectangles for each eye. Unequal physical quad dimensions are NOT proven. However, sizing used only the left horizontal FOV and ignored per-eye asymmetry, vertical bounds and IPD. LatchStereo now fits ONE common aspect-preserving rectangle against both actual eye frusta at entry. Both compositor quads keep identical pose and dimensions; anchoring stays fixed until the existing scene reset. Mono/Bink and native stereo projection are unchanged. This prevents entry-time clipping under the tested conditions; perceived equality in the headset remains PENDING.

## Verification

- Optimized x86 dual-weapon/TMD suite PASS, including actual independent palette attachment, stale/invalid/context gates, active-bone bounds and previous origin-dependent displacement regression.
- OLED D3D11 pixel/lifetime suite PASS (within dual suite).
- x86/x64 gaze/cinema/damage/mutant fixtures PASS; real D3D9Ex cinema pixel/state tests PASS. Legacy D3D9 HAL fixture NOT RUN: host rejects device with 8876086C.
- Common cinema geometry fixture PASS: asymmetric FOV/IPD, all corners in both frusta, eye-order parity, aspect, latch, invalid refusal and quaternion inversion.
- Build/binary ABI results and exact DLL identity recorded in follow-up candidate receipt.
- Steam runtime and physical PSVR2 for this new change NOT RUN. No FPS claim.

## One acceptance session

After controlled deployment, use TMD repeatedly while moving the head, walking and stick-turning with the right controller held steady; inspect gun-to-glove attachment, then support/release and reload. Start an engine cutscene and compare all four screen edges in each eye, full scene framing, subtitles and transition back to gameplay. Preserve accepted black-artifact, VFX and normal grip behavior. New candidate stays on its feature branch until physical acceptance; MAIN owns deployment.

## Built/deployed candidate

- Source: c3f60c0e838d017e6abc5ee9cb18c190d3eaeacc, dedicated fix/tmd-rigid-attachment-cinema-eyes branch.
- x86 PE32 /O2 DLL: C84569BFECD18E06289EF5C980827677F6746EA9BE11DC8570BBCA9E00F7D73A.
- Compiled ABI PASS: literal [eax+20Ch] factory and FName stack setup.
- MAIN DLL-only Steam installation completed after elevated CIM game/restore-writer checks. All profile file hashes verified; INI remains FC5D4E1C7AE0A9D3F8AA9E80116FEAC3E3629B10A8FC9AA82697C030CFC21C72. No settings/assets replaced, game not launched, no rollback timer.
- Previous 97F3D3FF backup: deployment/install-backups/20261103-103848. Deployment report: deployment/install-reports/20261103-103849.json.
- Private artifacts, full logs and deployment profile/plan: build/tmd-rigid-cinema/.
- New candidate physical PSVR2 acceptance PENDING. Only prior accepted improvement is on main.
