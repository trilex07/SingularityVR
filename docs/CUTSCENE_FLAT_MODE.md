# Cutscene and video flat presentation

This branch isolates automatic presentation changes from the native hands,
weapon, IK, and menu-input work.  The runtime state machine is in
`spikes/view_matrix/vr_scene_mode.h` and is sampled once from the existing
Present path.

## Signals

* `SetCinematicMode` is the existing UE3 script hook.  Its first frame-local
  dword is retained as `g_inCinematic`; no new object scan is performed.
* `IsVideoQuad` is the existing bounded Bink/full-frame classifier.  A frame
  that contains a confirmed video quad sets `g_sceneVideoSeen`.
* `g_vrUiMenu` remains the existing native GFx focus signal.  It classifies
  `MenuFlat`, but menus retain their current UI presentation and do not force
  the scene renderer into mono.
* `g_gamePaused` is recorded for transition diagnostics and does not by itself
  change stereo presentation.

## State and hysteresis

`GameplayStereo` is the default.  Two consecutive cinematic samples enter
`CutsceneFlat`; four consecutive clear samples return to gameplay.  A
confirmed Bink quad enters `VideoFlat` immediately and also returns after four
clear frames.  A single noisy cinematic sample therefore cannot flip the
presentation.  Optional transition logging is enabled with:

```ini
[Diagnostic]
SceneModeTrace=1
```

The log line includes the source signals and frame counter, for example
`SceneMode: GameplayStereo -> CutsceneFlat reason=1 ...`.

## Presentation

While `CutsceneFlat` or `VideoFlat` is active, `StereoPairBody` calls the
original draw once.  The full source image is consequently present in both
halves of the SBS target without applying an independent eye offset.  Normal
stereo duplication resumes automatically after the clear hysteresis window.
The existing video-specific rewrite still runs only for the classifier's own
quad; it cannot be armed by an unrelated fullscreen draw in the same frame.

No hand palette, arm IK, grip, weapon, controller, or input code is changed by
this branch.  The only shared rendering touch points are the Present-time
sample, the stereo-pair gate, and the existing video-quad classifier.

## Validation

`tests/test-vr-scene-mode.cpp` covers initial gameplay, two-frame cinematic
entry, four-frame exit, one-frame noise rejection, immediate video entry, and
menu classification.  It passes with:

```text
PASS: cinematic hysteresis, video mono, noisy signal, menu classification
```

The DLL is built for x86 with `/O2` and the PE header is `machine=0x14c`,
`optional=0x10b` (PE32).  A headset validation is still required to confirm
that the actual game cutscene and Bink paths trigger the transitions and that
the SBS compositor presents the intended mono image on both eyes.

## Known limits

The state machine is intentionally conservative: a cinematic that does not
call UE3 `SetCinematicMode`, or a video path that does not use the proven quad
signature, remains stereo until a new runtime signal is found.  Menu state is
classified for future policy and does not alter menu rendering in this branch.
