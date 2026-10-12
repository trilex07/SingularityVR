# Cutscene detection for flat cinema presentation — investigation + design

Research branch `research/cutscene-detection`. Side agent: no Steam,
no renderer changes, no deployment. Production `d3d9.cpp` and
`vr_scene_mode.h` untouched. New isolated helper:
`spikes/view_matrix/cutscene_detect.h` (+ offline test, PASS — see §6).

## 1. Confirmed detection signals (repo evidence)

- **UE3 `SetCinematicMode` hook (primary, engine-authoritative).**
  Detoured `ProcessInternal` watches the script table
  (`d3d9.cpp:19903`, `{ "SetCinematicMode", -1 }`); first frame-local
  dword retained as `g_inCinematic` (`d3d9.cpp:19981`). Run 136 saw it
  fire cleanly (`0x1` entering, `0x0` leaving); run 138 confirmed end to
  end (`CINE YES` through intro, `NO` at gameplay start); STATUS notes a
  later 92×ON / 88×OFF balance, i.e. the exit edge IS observable in
  recent builds. UE3 semantics (Epic docs: `ALevelScriptActor::
  SetCinematicMode` sets cinematic mode on all PlayerControllers) match:
  Matinee `SeqAct_ToggleCinematicMode` drives this flag, so
  Matinee-driven cinematics are covered without per-level timings.
- **Bink/video quad (secondary, draw-observed).** `IsVideoQuad`
  (`d3d9.cpp:17705`): 2-prim draw + sole draw in frame + `D3DFMT_L8`
  texture. Sets `g_sceneVideoSeen`, sampled per Present frame
  (`d3d9.cpp:19989`). Immediate `VideoFlat` entry is correct: a video
  frame is a single image and must never be stereo-split.
- **Camera-instance change (tertiary, clear path).** Run-211 fix
  (`d3d9.cpp:19374`): a camera instance change is an unambiguous level
  transition and clears a latched `g_inCinematic`. This is the backstop
  for any missed exit edge.
- **Menu focus / pause (context, never triggers).** `g_vrUiMenu` and
  `g_gamePaused` are sampled into the vote (`d3d9.cpp:19994`) for
  classification and trace only.

## 2. Gaps found (do not build on these)

- `docs/CUTSCENE_FLAT_MODE.md` claims `tests/test-vr-scene-mode.cpp`
  "passes", but that file exists in NO branch (`git log --all` empty).
  Treat the PASS claim as unverified until the test lands in-tree.
- `IsVideoQuad` dimension test is dead code: after the early
  `texFmt != kFmtL8` return, `(texFmt == kFmtL8 || ...)` is always true
  (`d3d9.cpp:17726`). Miss rate during real Bink playback unmeasured;
  run 167 logged 51 misfires/sec from the heuristic on non-Bink quads.
- Menus report `CINE YES` (HISTORY run 176 killed `MenuCameraFollow`
  over exactly this): a lone hook flag during menu focus is a KNOWN
  false positive, handled by demotion in the new helper (§4).
- CameraAnim-only sequences and `SetViewTarget`-only scripted cameras
  without `SetCinematicMode` have no signal in-tree (old ProcessEvent
  route structurally dead, HISTORY run 94). They stay stereo by design
  until a runtime signal is captured — conservative, no guessing.

## 3. Framework comparison (verified sources only)

- **Luke Ross R.E.A.L. mods (verified: GTA5 README, release notes):**
  per-cutscene viewing modes — dynamic stereo (default, adapts to
  camera FOV), 2D virtual screen (selectable), normal; user-cyclable at
  runtime. Pre-recorded video cutscenes go to the 2D screen (verified:
  KCD2 coverage). Maps to our `CUTSCENE VIEW: FLAT CINEMA / ALWAYS VR`
  rows; a future "dynamic stereo" middle mode is the natural extension
  if flat proves uncomfortable — out of scope for this branch.
- **UE pattern (engine semantics, not source-verified):** read the
  controller's cinematic flag (UE3 `SetCinematicMode` / UE4+
  `bCinematicMode`) rather than hooking video playback. Our hook-first
  design already follows this pattern.
- **UEVR (UE4/5, pattern-level only):** synchronized-sequential
  rendering with 3D UI handling; no UE3-applicable cutscene signal was
  source-verified, so nothing is imported from it.

## 4. Proposed state machine

```
GameplayStereo --(qualified cinematic x2 frames)--> CutsceneFlat
CutsceneFlat   --(exit edge | camera change | 4 clear frames)--> GameplayStereo
any            --(confirmed video quad, 1 frame)--> VideoFlat --(4 clear)--> GameplayStereo
menu focus     --(classification only)--> MenuFlat (existing UI presentation, never forces scene mono)
```

Qualification (`cutscene::Qualify`): hook alone ⇒ cinematic; hook +
menu focus without video ⇒ DEMOTED (reason bit, stays stereo);
video ⇒ cinematic regardless. Latch (`cutscene::Latch`): 2-frame
entry, immediate clear on exit edge / camera change, else 4-frame
clear hysteresis. Counters are O(1) integers — no measurable
frame-time overhead. Presentation unchanged: flat = original draw
once, same image both SBS halves, HMD pose still updates the camera
(head tracking preserved, run-138 behaviour); no weapon/glove/input
code in scope.

## 5. Limitations

- Cinematics that never call `SetCinematicMode` (pure CameraAnim /
  scripted-camera sequences) stay stereo until a signal is captured.
- Bink paths without the L8/sole-draw signature stay stereo; the
  classifier must NOT be broadened without A/B evidence (run-259
  regression lesson).
- Headset validation of the actual transitions is still MAIN-owned.
- `CutsceneAuto=0` remains the kill-switch convention for the hook.

## 6. Offline verification (this branch)

`tools/reference-tests/cutscene/cutscene_detect_test.cpp` covers
qualification truth table, 2-in/4-out hysteresis, one-sample noise
rejection, immediate exit-edge and camera-change clears with reason
codes, pause-never-cinematic, and the menu-demotion / video-override
pair. Result: `PASS: cutscene detect (...)` (x86 `cl /std:c++17`,
executed; artifacts removed, production code untouched).

## 7. Integration checklist for MAIN

1. Review `spikes/view_matrix/cutscene_detect.h` (pure logic, no hooks).
2. Wire `RawSignals` from existing globals only: `g_inCinematic`,
   exit-edge observation, `g_sceneVideoSeen`, `g_vrUiMenu`,
   `g_gamePaused`, camera-instance tracker. No new scans.
3. Feed `Latch::Engaged()` into the existing `UpdateSceneModeFrame`
   path behind `CutsceneAuto` (default preserves current behaviour);
   keep `SceneModeTrace=1` logging with reason bits.
4. Land the missing `tests/test-vr-scene-mode.cpp` (or point the doc at
   the new reference test) before quoting any PASS.
5. Validate on hardware, one variable per run: intro Bink → VideoFlat;
   Matinee cinematic → CutsceneFlat with head tracking; menu →
   never scene-flat; exit/level transition → GameplayStereo restore.
6. Do NOT broaden `IsVideoQuad`, touch weapon/glove/input systems, or
   add per-level timings. Fix the dead dimension test only with A/B
   evidence.
