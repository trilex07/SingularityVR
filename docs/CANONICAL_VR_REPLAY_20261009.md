# Canonical headset-route replay — 2026-10-09

This is the standard desktop replay of the user's recorded route. It is the
sequence to run after future DLL changes; it does not replace a physical
PSVR2 check when visual confirmation is required.

HEAD=f7be176
DLL_SHA=40CE5E13796BCDB2993537FA50456AC714E3148D95A76C334DF5B7DEB7ECB097
Frame=4323 (final stereo capture)
Eye=projection layer, 2 views; non-black 97.09% / 97.09%; eye age 0 / 0
WorldScale=80 (installed SingularityVR.ini)
CurrentPass=foreground projection/compositor capture
MenuState=statistics menu at final capture; route also entered gameplay and VR settings panel

## Source and replay

- Source: `C:\Users\Trilex\AppData\Local\SingularityVR\vr_sessions\session_2026-10-09_14-18-07.vrrec`
- Source samples: 5573, recorded duration 80.641 s.
- Live replay: 1420 acknowledged atomic batches, stride 4, speed 2 for the
  desktop run. Every mode transition and event edge was retained.
- Events: fire 8, melee 2, recenter 1, UI clicks 6.
- Transitions: panel → menu → gameplay, gameplay → menu, menu → panel, and the
  short load/unknown transitions at the end of the recording.
- Final input state was released: A/B/X/Y/menu false, both triggers/grips 0,
  both sticks centred. No replay input remained stuck.

## Evidence

- Replay report: `C:\Users\Trilex\vr-mods\deployment\selftest-20261009\canonical-replay-live-stride4.json`
- Final XR state: `C:\Users\Trilex\vr-mods\deployment\selftest-20261009\canonical-replay-state-final.json`
- Final SBS capture: `C:\Users\Trilex\vr-mods\deployment\selftest-20261009\canonical-replay-final_sbs.png`
- Per-eye captures: `canonical-replay-final_left.png` and
  `canonical-replay-final_right.png` in the same directory.
- Runtime log: `C:\Users\Trilex\AppData\Local\SingularityVR\view_matrix_2026-10-09_15-13-38.log`

The capture was focused at 90 Hz with 5074/5074 projection submits, no stale
eye frames, and one projection layer. The image visibly contains the same
two-eye statistics screen and right-controller laser/cursor path used by the
recorded menu route.

## One-command standard replay

```powershell
py -3 .\tools\xrsim_replay_vr_session.py `
  "$env:LOCALAPPDATA\SingularityVR\vr_sessions\session_2026-10-09_14-18-07.vrrec" `
  --dir "C:\Users\Trilex\vr-mods\xrsim-scratch-run" --stride 4
```

The Steam harness must be running first. The replay converts recorder angles
from radians to the simulator's degree command format, preserves pose/input
edges, acknowledges every command batch, and uses a monotonic file timestamp
so xrsim's 50 Hz poller cannot silently skip a frame.
