# CutsceneFlat mono-to-both-eyes presentation

Branch: `fix/cutscene-mono-presentation`, base `origin/main` @ `3c5ab21`.

## Problem (measured, not inferred)

In `CutsceneFlat`, `StereoPairBody` executes the cinematic draw ONCE across the
whole SBS backbuffer, but the captured frame is still stamped `dup=1,
stereo=1`. The XR bridge then submits one half per eye (`d3d9.cpp`, submit
loop), so each eye shows a different stretched half. The old
`CUTSCENE_FLAT_MODE.md` "full source image in both halves" claim is incorrect;
see also the unmerged `research/cutscene-detection` readiness note, which
describes the same symptom.

## Fix

Present-time composition + mono submission, behind the existing
`CutsceneFlatMode` switch (`ALWAYS VR` disables it):

1. `ComposeCutsceneMonoPair` (top of `CopyBackbufferToD3D11`, both capture
   paths): copies the full backbuffer to a lazily created temp surface, clears
   letterbox bars, and stretches the full image aspect-fit into the LEFT half
   (`cutscene_mono.h` fit math; e.g. 4096x2160 -> 2048x1080 at y=540).
   Fail-closed: any failure leaves the backbuffer untouched and the frame
   keeps stereo stamping.
2. `CopySlot::cineFlat` stamped only for a frame composed this Present.
   `VideoFlat` (Bink draws itself per eye) and menu frames never take it.
3. Bridge submits the same left-half crop (the validated menu crop
   coordinates, `left=0` always for cine) to both eyes' half-width swapchains
   with full-rect subImages. NIS stays off for cine frames.
4. `DrawCutsceneCinemaMask` skips composed frames (they carry their own bars);
   uncomposed flat frames keep the old matte.

Head tracking is preserved: mono-stamped frames submit live per-eye head
poses (`g_eyePose`) with the eye FOVs, so the cinema follows head orientation
without stereo parallax. Detection (hook arg, level-transition clear, 2-in /
4-out hysteresis) is unchanged; Matinee camera cuts hold `SetCinematicMode`,
so no premature return is expected.

## Verification

- `tests/test-cutscene-mono.cpp` PASS (SBS fit rect, bars, odd dims,
  degenerate inputs, compose predicate matrix).
- `tests/test-vr-scene-mode.cpp` PASS (unchanged detection behaviour).
- `tests/validate-runtime-policy.ps1` PASS.
- x86 PE32 /O2 build + manifest/SHA provenance per candidate below.

## Candidates (separately hashed)

1. MERGED BASELINE: clean `origin/main` @ `3c5ab21`, SHA
   `D8928180262706A95972D13B728A1867F1EAB1757DCFD6C4A32D99EFBFBEC1BE`
   (built in `SingularityVR-main-check`, regular non-Performance).
2. CUTSCENE CANDIDATE: baseline + this change, SHA recorded in
   `spikes/view_matrix/d3d9.build.json` of this branch at test time.

## Round-2 physical findings (candidate 1D6FD331, PID 20128, new game)

- Single APPCRASH at first video start; clean retry played through. No dump;
  candidate is behavior-identical to baseline at video time, so no mechanism
  attributed. Treat as flake pending recurrence.
- Helicopter Matinee: FPS no longer dropped (uncontrolled observation, cause
  undetermined), but still no cinema view. Root cause in the log:
  `level transition: camera 1A4B9300 -> 2D606900, CINE was ON - clearing it`.
  The run-211 level-transition clear fires on a Matinee camera CUT, so
  `CutsceneFlat` never engages and the mono composition never runs.
- Fix (this branch): gate the clear on pawn continuity + menu focus
  (`CameraChangeClearsCine`, unit-tested). `ResetVrAfterCameraLoss` untouched.
- Still open, separate work: inverted head response during cinematics (log:
  `yaw asserted by the ENGINE ... cine ON`), visible gloves/weapon, movement
  allowed, Bink size request, round-1 FPS drop cause.

## Physical test script (one coordinated window, no mid-game chat)

1. Deploy BASELINE, play to a Matinee cutscene: record stereo-split symptom.
2. Deploy CANDIDATE, same cutscene: both eyes same complete image, correct
   aspect, head orientation tracked, no squeeze/clip/double.
3. Intro Bink separately: unchanged per-eye video.
4. Camera-cut-heavy cinematic: no flicker back to stereo mid-scene.
5. After cinematic: automatic return to stereo gameplay.
6. Regression: pause RTT open/options/resume, weapon tracking, held-button
   FPS, dynamic shadows.
