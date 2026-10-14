# Own-wrist anchoring experiment

PID21056 physical feedback: frontend menu is single with matching eyes, but its
background is incomplete. Cinema pause was not exercised. Small FPS dips remain,
unrelated to L2/R2 in this trial. Native gameplay speech is still English. All four
leased installation files and all 1886 WAV originals were restored and independently
hash-verified after exit. No next Steam deployment or launch without fresh `start`.

## Evidence and hypothesis

The existing controller transform rotates/scales about the gun anchor, not the
native wrist. Skin constants are inverse-bind times animated joint. Reconstructing
each joint from the verified noTMD reference, then applying original local230,
reveals the wrist's residual offset after the gun-pivot controller transform.

Eight bounded samples were recorded; five were cinematic, only three gameplay.
They must not all be described as near-face gameplay captures. Frame17519 has no
camera ride and reconstructs residuals of 49.997UU left and 40.844UU right
(approximately 0.76m/0.62m at this user's 65.625UU/m). Frames17194/17356 used a
ride, which the old probe did not copy: their replay assumes the copied controller
transform equals the ride. They are weaker evidence, not independent ride proof.

Hypothesis: using each reconstructed wrist as its own controller pivot removes
the measured gun-anchor residual. Falsifier: the tracked wrist still misses the
physical controller despite the recovered joint reaching the logged target. That
would require checking bind/local/coordinate convention or controller origin,
not guessing another distance offset. New probe records the actual camera ride
and captures only GameplayStereo, outside menus/pause/cinematics.

## Narrow implementation

`NoTmdWristAnchorFix` is default OFF. It applies only with NoTmdPaletteFix,
GloveIsolate2, both tracked hands valid, exact noTMD gloves in gameplay, and no
support grip. Controller rotation/scale is preserved; only pivot translation is
adjusted. Undo the actual BuildGunC/BuildTmdC ride, not its live-controller
surrogate. Invalid matrices fail closed before upload.

The existing left parking shift is bypassed only in this scope because it would
move the newly anchored wrist away from its controller. The user's INI value12
is retained. Gun, TMD, support grip, shadows, world, menus and cinema paths are
not changed by this flag. Existing palette restoration remains authoritative.

L1: synthetic reconstruction/rotation/scale/ride cancellation/fail-closed/gating
tests and the three-record private replay pass. Corrected replay residual is
below0.00002UU. This is algebraic validation, NOT GPU pixel or PSVR2 acceptance.
L2 PASS: sourcea23d7e9, x86 PE32 `/O2`, DLL SHA-256
E213B216F90CF5545862D8850574D5CD7F82A480983847D4F989507E3E3E58A5.
Offline four-file artifact `deployment/hands-dubbing/wrist-anchor-test-artifact`
adds NoTmdWristAnchorFix1 alongside the previous trial's four experiment flags.
Existing fake-install tests pass: WhatIf, path refusal, corrupt backup rejection,
user-INI conflict preservation, exact restore and frozen UI. No real installation
changed and no Steam game launched. Physical own-wrist acceptance remains pending.

## 2026-10-14 Steam evidence (physical feedback pending)

Fresh `start` authorized sourcea23d7e9 / E213B216 candidate. MAIN verified exact
four installed hashes and loaded proxy paths, OpenXR ready, guard and observer.
PID23584 logged cinema pause reason32 -> MenuFlat and back to CutsceneFlat, then
GameplayStereo. Its hand path was blocked at the existing position-valid gate;
there are no wrist samples in that session. Do not claim it tested wrist execution.

The user restarted into PID36340 while the same candidate was still leased.
No MAIN relaunch or file replacement occurred. First log was already saved by
the observer; second log was copied separately after exit (the restarted game
exited before a second observer could start). Second log proves wrist-correction
execution and eight GameplayStereo records with actual camera ride copied.
All records have moved0; moved1/2 remain synthetic-only cancellation checks.
Offline reconstruction of the *captured posed constants* reaches the logged
tracked targets within approximately0.001UU (logged precision), instead of the
previous approximately41–50UU residual. This is constant-content validation,
not pixel/physical controller-origin or glove-mesh acceptance.

Guard restored all four originals after both sessions closed; MAIN independently
verified installed and backup hashes, UI5B,107DDS and frozen config/forwarder.
No new SIDE voices or the semantically faulty42 native-bank slice were installed.
Private evidence: `deployment/hands-dubbing/wrist-anchor-live-20261014-002420`.
User's perceptual feedback is still required; no feature marked physically fixed.
