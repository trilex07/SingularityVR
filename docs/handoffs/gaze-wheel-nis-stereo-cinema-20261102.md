# Gaze wheel, gaze NIS and native stereo cinema — integration handoff

Branch: `feature/gaze-wheel-nis-stereo-cinema`.
Base: `2e9d5f326fa0243b3950f49223e569731601c532` (accepted origin/main).
Reference: Darkness2VR `3c19e744`, `gaze-nis-main-integration` worktree.

## Scope and state

L1/L2 PASS; Steam/game/OpenXR/physical PSVR2 NOT RUN. All three feature
switches default OFF. Seeker reload, accepted hands/TMD, native weapon aim,
ammo, localization and installed Steam files are untouched. This is a candidate,
not a physical acceptance claim.

- Native real-time cutscenes: two offset native-camera scene views on the same
  anchored LOCAL cinema screen. Uses existing measured ROW/COL convention,
  CTAB-labelled ViewProjection constants, actual tracked IPD and existing UU/m.
  No HMD rotation is injected into the authored camera. LEFT/RIGHT quad layers
  consume separate source-eye rectangles at the original camera aspect.
- Pre-rendered Bink movies remain mono cinema. A flat movie does not contain
  a second camera view; this patch does not invent 3D conversion.
- Weapon wheel: gaze uses the actual rendered eye-0 wheel centre/radii, 80 ms
  dwell, existing ownership and existing R3-release confirmation. Missing,
  stale (>100 ms), invalid or blink samples use original controller selection.
- NIS: existing same-resolution sharpening, with 25% peripheral strength, 62.5%
  transition strength and configured strength at gaze centre. Nested squares
  overwrite from the original isolated eye source, never re-sharpen a prior pass.
  Upscale, menus and cinema keep their existing paths. Invalid gaze produces
  exactly the existing uniform NIS result. Order: capture -> OLED -> NIS -> XR.
  This adds compute passes; no FPS, scene-foveation or VRAM benefit is claimed.

## Controls and dependencies

VR Settings > DISPLAY NIS/OLED now includes GAZE WHEEL, GAZE SHARPEN and
STEREO CINEMA. Existing navigation, laser and persistence dispatch are reused.

```ini
[EyeTracking]
Enabled=1
WeaponWheel=0
NisSharpen=0
[Render]
CutsceneStereoCinema=0
```

Set individual switches to 1 for the controlled candidate. Gaze sharpening
requires `[Upscaling] Mode=NISSharpen`; it does not silently change NIS mode.
Master Enabled=0 disables gaze; restart game/bridge for provider opt-out.

Requires the existing x64 PSVR2 Toolkit loader/backend with optional
`psvr2_toolkit_gaze_status` support, tracking enabled/calibrated by the user.
No Toolkit binary is added. The existing trigger bridge is extended with
`--gaze`, a current-user-only read-only game mapping, and nonblocking timeout-0
polls on its existing thread. Missing export/mapping/driver is not a trigger
failure. No gaze coordinates are written to the logs. Trigger calls retain their
existing serialization. A bridge restart while the same game still holds its
mapping requires restarting the game/bridge; stale samples fail closed meanwhile.

Cinema prerequisites: existing duplicate draw, tracked eyes, native-camera
cutscene classification and verified matrix convention. No guessed register or
matrix convention fallback. One receipt per refusal class identifies missing
CTAB/native matrix/convention/IPD. Unclassified scene shaders draw natively;
coverage of particles/post effects across real cutscenes remains a runtime risk.
Initial cutscenes before any convention witness can remain mono. Unsupported
content must be assessed before production enablement, not described as PASS.

## Validation recorded 2026-11-02

- `tools/tests/test-gaze-cinema.ps1`: x86 and x64 /O2 PASS. Shared ABI sizes,
  seqlock, stale/future/NaN/blink rejection, asymmetric FOV, eye-region bounds,
  wheel sectors, dwell, R3 release and missing-provider fallback.
- Real D3D9Ex pixel fixture, both architectures: 138 pixels/eye, horizontal
  centroids 89.63 and 70.37 (distinct stereo views). ROW/COL transpose parity;
  original constants and scissor restored on success and injected draw failure.
- Legacy D3D9 HAL fixture: NOT RUN, host CreateDevice returns 0x8876086C.
  D3D9Ex success does not establish legacy D3D9 runtime acceptance.
- `tools/fx/test-nis.ps1`: PASS. D3D11 WARP NIS regional compute changed 914
  pixels; outside-region periphery and invalid-eye uniform fallback exact;
  scale/sharpen isolation, resize/lifetime and three wiring regressions PASS.
- `tools/tests/test-dual-weapon-tmd.ps1`: PASS, including reload, measured aim,
  hands/TMD, WeaponLocker and OLED resource/failure and GPU reference fixtures.
- `tools/tests/test-independent-hands-shot-haptics.ps1`: PASS.
- `tools/psvr2-bridge/test-pipe.ps1`: PASS, actual x86/x64 authenticated IPC,
  pistol/shotgun/rifle/sniper/Off/Shutdown, hardware OFF.
- Production `spikes/view_matrix/build.ps1`: x86 PE32 /O2 PASS (existing static
  analysis warnings remain). `tools/psvr2-bridge/build.ps1`: x64 PE32+ /O2 PASS.

Binary identity (private local build, do not commit binaries):
- `spikes/view_matrix/d3d9.dll` SHA256
  `F60E1BD047B6481DEDF5593997E5793E1EBB9FDB696BFBF194C75619B760E35F`.
- `deployment/psvr2-bridge-x64/SingularityPsvr2Bridge.exe` SHA256
  `C4D002A3F09258F311EFF9CFB4FF20ABAD4CEB9CF3967E25A80212DFAF335271`.
The build provenance records the accepted base plus dirty source enumeration;
the committed source matches that compilation. No installed profile was replaced.

## One grouped MAIN acceptance gate

MAIN first backs up and verifies DLL, INI and existing x64 bridge, then installs
only the reviewed artifacts with the game closed. Retain current settings and
enable the three candidate switches. No save edits or ammo injection.

1. At a native cinematic, verify stable cinema position with distinct depth in
   both eyes, no doubled actors/particles or crop, then skip/pause/resume and
   return to gameplay. Bink remains correctly framed mono.
2. Hold R3, look at several owned weapons, release R3 to select. Check controller
   fallback with unavailable gaze, menu laser, and no unsolicited weapon change.
3. In NIS SHARPEN, compare gaze OFF/ON, look across image edges and centre;
   confirm both eyes, no seam/black patches. Record frame timing rather than
   claiming a performance gain. Confirm settings survive restart.
4. Briefly confirm accepted hands, TMD, two-hand grip, reload and haptics.

STOP/restore on stereo corruption, unstable tracking, persistent GPU/resource
errors or gameplay regression. Keep any failed feature OFF independently.
Physical acceptance and actual gaze-provider operation remain PENDING.
