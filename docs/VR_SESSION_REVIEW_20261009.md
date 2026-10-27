# VR session review — 2026-10-09

## Recording inventory

All four original recordings in `%LOCALAPPDATA%\SingularityVR\vr_sessions` were
read-only parsed and passed the offline trigger-leak validator. Two short captures are
panel-only; two longer captures cover the menu/gameplay route. The `canonical-first400`
and `canonical-first4000` files under `deployment/runtime-test-current` are derived
prefix clips of the 14:18 recording, not additional user recordings.

| Recording | Samples / duration | Route and events | Prompt evidence |
|---|---:|---|---|
| `session_2026-10-09_12-01-40.vrrec` | 79 / 0.875 s | Panel only; no events | No prompt flags |
| `session_2026-10-09_13-14-53.vrrec` | 321 / 3.563 s | Panel only; 1 recenter | No prompt flags |
| `session_2026-10-09_13-14-58.vrrec` | 10,368 / 125.609 s | 7 fire, 11 UI clicks; repeated panel/menu/gameplay and brief unknown states | 280 prompt-flag samples; `prompt_action=0` throughout |
| `session_2026-10-09_14-18-07.vrrec` | 5,573 / 80.641 s | 8 fire, 2 melee, 1 recenter, 6 UI clicks; repeated gameplay↔menu transitions | 164 prompt-flag samples; `prompt_action=0` throughout |

Both controller pose streams changed on every adjacent sample in all four files. This
proves recorded pose data exists; it does not prove that the hands were independently
controlled during capture or that either rendered hand followed its pose. The two
longer files contain useful lifecycle and prompt-state regression inputs. Both have now
been replayed through Steam+xrsim on the current line: the 14:18 recording applied
1,420/1,420 stride-4 batches, and the 13:14 recording applied 2,625/2,625 stride-4
batches in 161.635 seconds. The longer route exercised 22 state transitions, seven
fire events and eleven UI clicks. It ended FOCUSED with two projection views and zero
mono/stale submits or runtime errors. Its delayed 1,200-frame CPU window measured
10.447 ms/frame in the Present hook, 9.100 ms in `xrWaitFrame`, 0.312 ms residual, and
0.009 ms/frame in `TrackFpCaster` across 105,846 calls with zero candidates or buffer
queries. This remained normal xrsim pacing and did not reproduce the reported PSVR2
menu hitch. Runner `steam-xrsim-run-20261009-202126.json` confirms restoration after
the run; the source recording was not modified.

## Bounded arm-palette trace

Both long routes were also replayed with `Diagnostic/ArmPaletteTraceFrames=64`; the
Windows INI API confirmed the setting was read from the correct section. Both runs
completed with the same xrsim stereo/error checks. Neither runtime log contained an
`ArmPaletteTrace` entry or a `TMD RAISED` transition. The 13:14 file's short
gameplay `weapon=0` spans do not establish that the TMD was raised. These recordings
therefore do not exercise the exact 7,444-vertex TMD arm sections the trace targets and
cannot prove the rendered-hand path. This is a negative test-coverage result, not
evidence that the user's hand defect is fixed or that the arm draw is absent in a live
TMD scene.

Artifacts: `deployment/agent-test-bridge/xrsim-reports/hands-trace-20261009-replay-delayed.json`,
`hands-trace-long-20261009-replay.json`, runner reports `steam-xrsim-run-20261009-203343.json`
and `steam-xrsim-run-20261009-203655.json`.

The recorder output from the user's Steam run is valid:

- source: `C:\Users\Trilex\AppData\Local\SingularityVR\vr_sessions\session_2026-10-09_14-18-07.vrrec`
- size: 869,404 bytes
- decoded samples: 5,573
- decoded CSV: `deployment/session_2026-10-09_14-18-07.csv`
- screenshots: `capture_2026-10-09_14-17-26.bmp` and `capture_2026-10-09_14-17-35.bmp`

## Evidence

Both left and right pose streams changed on every recorded sample. The left controller was therefore
present in the OpenXR input stream; the report of a visually frozen left hand is in the render/palette
path, not a missing controller recording.

The context-prompt flag was set on 164 samples (61 with gameplay+prompt and 85 with gameplay+UI-valid+
prompt, plus the remaining transition samples), but every sample had `prompt_action=0`. The native
draw classifier did not publish an action identifier. The runtime now records the confirmed gameplay
context prompt as interaction/R1 until the GFx alias hook is resolved.

The later Steam+xrsim replay log `deployment/agent-test-bridge/final-20261009-194135/view_matrix.log`
adds renderer-path evidence: while `gameplay=1`, the classifier matched two stride-20 draws
(`prims=24` at `(-0.021,-0.138)` and `prims=16` at `(-0.119,-0.138)`), and the context icon draw
ran repeatedly. The captured BMPs in that collection are main-menu frames, not frames from the
matches. The log does not identify the text/action or a chest/fountain state, so it cannot separate
a real contextual prompt from a false geometric match. Keep P3 BROKEN until a named interaction
and its inactive interval are captured with the submitted pixels.

A retrospective check of the workspace-level xrsim capture archive found three additional
SBS frames named `look-chest`, `look-chest2`, and `look-chest3`, captured at frames 7187,
8045, and 8992 during the older SHA `73A0FBC2…` run. They show an orange chest-like box
in the world; their metadata contains no game-object or interaction state, and all button
and trigger values are zero. The associated 18:37:55 log recorded 18 generic stride-20 HUD
candidates but zero `context prompt MATCH` and zero screenshot requests. The images are
useful visual scene references, but do not establish that the box was interactable or that
its prompt was active. A later 18:50:48 log recorded 16 centered matches at `(-0.057,
-0.138)`; the nearby `prompt-live` xrsim image has no named target and was not captured
on the exact matched draw. These artifacts still cannot map the classifier to a native
action or prove the active/inactive visibility transition.

A cross-log check found the stable bottom candidate
`prims=28 stride=20 clip=(-0.075,-0.783)` in `view_matrix_2026-10-09_18-28-32.log` and the four logs at
20:11:40, 20:17:48, 20:21:28, and 20:36:57. Each occurrence is adjacent to a live
gameplay hook gate, but the current source explicitly records this geometry as the NOTE
close button measured at `(-0.075,-0.755)`. It is therefore a known non-world-interaction
candidate; it must not be adopted as the chest/fountain anchor. The current center-band
classifier (`-0.25 < y < -0.05`) is also unproven: the 18:50:48 matches in that band have
no synchronized named target. The required next evidence remains a frame-synchronized,
named interaction with its inactive comparison and action glyph.

Archive references: `../xrsim-scratch-run/capture/look-chest_sbs.png`,
`look-chest2_sbs.png`, `look-chest3_sbs.png`, `prompt-live_sbs.png`, and corresponding
JSON metadata; runtime logs are `%LOCALAPPDATA%\SingularityVR\view_matrix_2026-10-09_18-37-55.log`,
`view_matrix_2026-10-09_18-50-48.log`, `view_matrix_2026-10-09_18-28-32.log`,
`view_matrix_2026-10-09_20-11-40.log`, `view_matrix_2026-10-09_20-17-48.log`,
`view_matrix_2026-10-09_20-21-28.log`, and `view_matrix_2026-10-09_20-36-57.log`.

The `Shadow` flag was set on all samples because `VrShadows=1` was enabled. This is a configuration
flag, not proof that a shadow pass rendered. The runtime counters in the same session were:

```
shadow depth/skip/eye-error = 0/0/0
```

No classified shadow generation or sampling draw was observed in this run, so the session does not
provide evidence that visible shadows were present.

## Changes deployed after review

1. `PrepareIndependentLeft` no longer requires the primary/right-hand aim deviation to be valid.
   A brief right-pose loss or transition can no longer suppress an otherwise valid left-hand draw.
2. A confirmed native context prompt now records action `1` (interaction/R1) instead of leaving the
   action field at zero.
3. The shipped Steam INI now enables `IndependentHandsPrototype=1`, keeps `ShowControllerArms=0`
   (glove-only), and explicitly keeps `ArmIKMode=0` until the next headset check.

Build: x86 PE32 `/O2`

Deployed DLL SHA-256: `524B2A4E27AF81B25604D32D38C749A8AB2AFAEA3AB28AFE87ED9A6A4FFA3BE0`

Rollback backup: `deployment/install-backups/20261009-142721`

## One-shot prompt capture follow-up

The source now has opt-in `[Diagnostic] CaptureFirstContextPrompt=1`, which requests one
bounded BMP capture on the first context-prompt geometry match; the default remains off.
Candidate SHA-256 is `10EC5F41C71D6FBAB2D7E5417A2761C3A776BB619D8712C995C7E3452C8EBFFD`.
The first two runner attempts were made in the restricted shell and reported a locked
file; a Win32 probe returned access-denied (5), and the candidate installed successfully
under the approved elevated runner. That run exposed and fixed the runner's incorrect
test-bridge root path. Its mandatory preflight passed with built/deployed SHA equality,
the Steam+xrsim replay launched and applied 1,420/1,420 batches, and the runner restored
the baseline DLL `4ABF8081CEC1FEFE4768C0C180CC704C85D399DB8A825105BFC2C715B1CBB2EC`.

The diagnostic setting loaded, but this replay produced no `context prompt MATCH`, no
screenshot request, and no new BMP. Its bounded gameplay HUD census logged stride-20
draws at x≈−0.64, outside the centered classifier. The replay ended FOCUSED with two
projection views and zero runtime errors; this does not establish behavior at a named
chest/fountain interaction. P3 remains BROKEN. Artifacts: runner report
`deployment/agent-test-bridge/prompt-capture-reports/steam-xrsim-run-20261009-205833.json`,
replay report `deployment/agent-test-bridge/xrsim-reports/prompt-capture-20261009-replay.json`,
and the test-bridge collection under `../deployment/agent-test-bridge/20261009-210125/`.
