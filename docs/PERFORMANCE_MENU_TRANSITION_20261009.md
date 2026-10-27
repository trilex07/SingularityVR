# Menu transition performance — 2026-10-09

The captured Steam/PSVR2 session in
`%LOCALAPPDATA%\\SingularityVR\\view_matrix_2026-10-09_13-14-06.log` shows the hitch is
specific to returning from gameplay to the native menu:

* normal gameplay window: 87.3 FPS with 0.06 ms/frame of mod gameplay-hook work;
* first transition window: 60.0 FPS / 16.68 ms;
* main-menu transition window: 20.9 FPS / 47.82 ms, with 46.53 ms in the residual
  Present/render path and 0.90 ms in gameplay hooks;
* the same window rendered 442 split draws/frame, 26 offscreen targets, and a private
  1280x720 GFx menu candidate while the menu capture state changed.

The evidence rules out the XR copy path and the VFX host as the direct cause: copy was
0.01 ms and NIS was OFF. The menu scene/render transition is GPU/Present-bound, with a
smaller avoidable cost from pawn/weapon discovery continuing after gameplay ended.

The production change keeps gameplay behaviour unchanged but quiesces
`SolvePropertyLayout`, `RefreshArmedState`, `UpdateFgLatch`, `ResolveGameplayOffsets`,
`ReportEyeHeight`, and `ApplyMuzzleFxSuppression` whenever `InGameplay()` is false or a
native menu/VR Settings panel is active. Cached pointers are not dereferenced by these
scans during menu/loading; the path resumes on the first stable foreground frame.

Build and desktop checks:

* x86 PE32 DLL built with `/O2`;
* pause/menu, interaction, recorder, and NIS tests pass (NIS: 167/167);
* deployed DLL SHA-256:
  `D12EAB88798D826713C8B593FCD30CBAA44EF5717B6F5408A6D5558FDA10F4E4`;
* installed `SingularityVR.ini` restored after deployment (user tuning preserved),
  SHA-256 `556476261B4A4D2C8908384BBCC00C1311F1E5E2440C228EFDD4D0F7A16A7A2B`.

The installed build still needs one headset A/B transition measurement to quantify the
remaining GPU-bound menu cost. The next useful capture is: gameplay → pause/menu → main
menu, wait 5 seconds, then quit, with the same render scale and menu overlay settings.

## Delayed xrsim CPU window — 2026-10-09

To check whether the mod's render hooks explain that hitch, a bounded sampler can now
wait for a configured number of Present frames before collecting its 1,200-frame window.
`HotPathSamplerDelayFrames` defaults to 0; its maximum accepted value is 100,000. The
test used `HotPathSampler=1` and `HotPathSamplerDelayFrames=5300`, placing the window
over the first gameplay→menu transition in the saved session.

The xrsim replay completed all 1,420/1,420 batches and remained FOCUSED, stereo, and
error-free. In the delayed window:

* Present hook total: 9.444 ms/frame; `xrWaitFrame`: 8.339 ms/frame;
* inclusive-stage residual inside the Present hook: 0.391 ms/frame;
* `TrackFpCaster`: 0.018 ms/frame across 224,717 calls, with zero candidates or D3D
  buffer queries;
* draw-hook estimate: 0.220 ms/frame; other Set hooks: 0.118 ms/frame.

The sampled CPU hooks contribute little in this controlled replay, and `TrackFpCaster`
does no candidate buffer work there, so it is a poor next target for the menu hitch.
The xrsim window ran at normal paced frame times and did not reproduce the physical
PSVR2 hitch; it cannot establish the hook cost under that runtime or measure GPU queue
and SteamVR compositor stalls. The headset A/B capture remains necessary to locate the
residual cost on the target runtime.

Runtime artifacts: `deployment/agent-test-bridge/xrsim-reports/hotpath-delay5300-session-replay.json`
and `deployment/agent-test-bridge/xrsim-reports/steam-xrsim-run-20261009-201138.json`.

The longer 13:14 recording also completed through Steam+xrsim (2,625/2,625 batches,
161.635 s) with the window delayed 2,700 Present frames to overlap its first return to
menu. That window measured 10.447 ms/frame Present hook, 9.100 ms `xrWaitFrame`,
0.312 ms residual, and 0.009 ms/frame `TrackFpCaster` across 105,846 calls (zero
candidates). It likewise stayed near the simulator's 90 Hz pacing. Artifacts:
`deployment/agent-test-bridge/xrsim-reports/long-route-20261009-replay-retry.json` and
`deployment/agent-test-bridge/xrsim-reports/steam-xrsim-run-20261009-202126.json`.
