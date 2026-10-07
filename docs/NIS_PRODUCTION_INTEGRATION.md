# NIS production integration manifest

Branch: `research/nvidia-vr-upscaling` (frozen baseline `de83cae`).
Target (when stable): `integration/nis-on-latest-runtime` from the exact
stable `origin/fix/vr-menu-input-grip-performance` commit, by selective
cherry-pick of the NIS commits below — never a blind merge.

NIS commits to carry (in order):

- `bba09be` — vendor official NIS v1.0.3 + per-eye bridge math (new files only)
- `abefbee` — D3D11 runtime (new file only)
- `a30c388` — bridge integration into d3d9.cpp (shared-file edits start here)
- `00b2a86` — tests + experiment deployment (new files only)
- `fd55310` — embedded bytecode + fallback (new files + runtime header)
- `01a1b3f` — bridge adapter + d3d9.cpp slim-down (shared-file edits)
- `de83cae` — engage/seam tests + counters (tests + 2 log lines)

Only `a30c388` and `01a1b3f` touch the shared `d3d9.cpp`. Everything else
is new files the production branch has never seen.

## Minimal production call surface

After integration, d3d9.cpp should contain exactly one NIS global
(`static NisBridge g_nis;`) plus these calls — all policy lives in
`nis_bridge.h` / `nis_runtime.h` / `nis_upscale.h`:

| # | Area | Production function | NIS call | Why |
|---|------|---------------------|----------|-----|
| 1 | includes/global | top of d3d9.cpp | `#include "nis_upscale.h"`, `#include "nis_bridge.h"`, embedded CS headers, `static NisBridge g_nis;` + `NisLogAdapter` | Owns all NIS state in one symbol; no scattered globals |
| 2 | INI parse | `LoadIniSettings()` | parse `[Upscaling]` Mode/RenderScale/OutputScale/Sharpness into `g_nis` + `g_nisRenderScaleF` | Off default; unknown mode → Off; all values clamped |
| 3 | AutoRes | `InstallAutoResolution()` (early read) + `BuildAutoCmdLine()` (apply) | read RenderScale at DllMain time; scale cached full size via `ComputeRenderSBS` | RenderScale must reach UE3's command line; cache keeps full size |
| 4 | bridge init | `UpdateFromHeadset()` submit setup | `NisBridge_OutputEye` + lazy `NisBridge_InitIfNeeded` (embedded → HLSL fallback) + `NisBridge_ShouldEngage` + `NisBridge_EnsureFrame` | Output from recommended, init once, engage only on SBS true-stereo |
| 5 | stereo frame | `UpdateFromHeadset()` eye copies | `NisBridge_ProcessStereo` then `CopyResource(outTex → swapchain)` per eye; native halves on `false` | 1 stage + 2 dispatches + 2 final copies; seam never in kernel |
| 6 | timing | end of submit, after `xrEndFrame` | `NisBridge_PollTiming` | harvest timestamps without stalling |
| 7 | toggle | `Hook_Present()` hotkeys | `N` cycles Off→NIS→NISSharpen (one log line) | single-session A/B; RenderScale still needs relaunch |
| 8 | perf | `Hook_Present()` per-second window | NIS `[MEASURED GPU]` line + `NisBridge_ResetStats` | mean/peak GPU, stage mean, sizes, lifetime counters |
| 9 | shutdown/reset | device reset/exit path (add if missing) | `NisBridge_Shutdown` | release stage/outputs/queries on reset; currently process-lifetime only |

New files production gets verbatim (no gameplay interaction):
`nis/` (official HLSL + config + licence + embedded DXBC headers),
`nis_upscale.h`, `nis_runtime.h`, `nis_bridge.h`,
`tests/test-nis-upscale.cpp`, `tests/test-nis-bytecode.cpp`.

## Conflict map vs `origin/fix/vr-menu-input-grip-performance` (head `9a765a1`)

Method: `git diff 2382ff7..research` vs `2382ff7..origin/production`
for `spikes/view_matrix/d3d9.cpp` (the only shared file; 13 NIS hunks,
19 production hunks, **zero textual overlap**).

LOW (new symbols / untouched regions — apply cleanly):
- #1 includes/global — production include hunk is at file top for other
  headers; NIS adds adjacent lines only.
- #4/#5 submit setup + eye copies in `UpdateFromHeadset` — production's
  only `UpdateFromHeadset` hunk is ~700 lines away (headset sync).
- #6 timing poll, #7 N-toggle — regions production has never edited
  (no other hotkey uses `N`).
- #2 INI block — production never touched `LoadIniSettings`; it only
  appended 4 lines to `SingularityVR.ini` (NIS documents `.ini.example`,
  a different file).

MEDIUM (same function, adjacent but non-overlapping — expect fuzz, verify):
- #8 perf line + #9 reset in `Hook_Present` per-second window — production
  edited the shadow-projection log ~20–140 lines away in the same window.
  Cherry-pick should apply; confirm the NIS line still sits after the
  frame-copy log and resets still pair with the other accumulators.

HIGH (correctness-critical even without textual overlap — review line by line):
- #3 `BuildAutoCmdLine` — sole owner of what UE3 renders; production
  untouched, but any future production edit here must preserve the
  RenderScale application and the explicit-`-ResX`-wins rule.
- #5 eye-copy branch — production owns swapchain/submit semantics
  elsewhere; the NIS branch must keep native behavior bit-identical when
  `nisFrame` is false (regression: Off build ≡ production native path).

## Integration rules (binding)

Production behavior ALWAYS wins for: menu, GFx, cursor, laser, hands, IK,
grip, shadows, prompts, Triangle, recenter, camera. NIS wins only for:
upscaling, render-resolution scaling, NIS D3D11 resources, NIS frame
processing, NIS GPU timing. Never restore older production code from NIS
side; resolve conflicts by keeping production hunks and re-applying the
numbered NIS calls above.

## A/B presets (prepare only — no headset session yet)

- A baseline: `Mode=Off`, `RenderScale=1.0`, `OutputScale=1.0`
- B performance: `Mode=NIS`, `RenderScale=0.77`, `OutputScale=1.0`, `Sharpness=0.5`
- C quality: `Mode=NISSharpen`, `RenderScale=1.0`, `OutputScale=1.0`
- D aggressive (optional): `Mode=NIS`, `RenderScale=0.70`, `OutputScale=1.0`

Gate the session on the production agent's stable signal; never mix the
first NIS session with menu/input debugging.
