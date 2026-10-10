# VR session QA — 2026-10-09

The headset run produced a valid recorder file:

`C:\Users\Trilex\AppData\Local\SingularityVR\vr_sessions\session_2026-10-09_12-01-40.vrrec`

The dump/replay checks reported 79 samples (`first_frame=1`, `last_frame=79`) and no trigger leakage. The stream contains no `recenter`, `ui_click`, `fire`, or `melee` events. That is expected for this capture: the log records `L3+R3 chord: ... recording=off`, so L3+R3 saved screenshots and requested recenter while the session recorder was not running. Recording is a separate `RECORD SESSION` row in VR Settings.

The same log confirms that both controller action/pose paths were active and that the VR Settings pages were opened (`VR SETTINGS`, `CONTROLLER`, `COMFORT`, `DISPLAY`, `ADVANCED`, and `TMD ALIGN`). The two L3+R3 screenshots were saved at:

- `C:\Users\Trilex\AppData\Local\SingularityVR\capture_2026-10-09_12-02-26.bmp`
- `C:\Users\Trilex\AppData\Local\SingularityVR\capture_2026-10-09_12-03-15.bmp`

Native fountain/chest prompt hooks remain unresolved in that run; the reload prompt was supplied by the mod's generic context prompt path. A new capture intended to validate UI clicks should first enable `RECORD SESSION` in VR Settings, then perform the route, and finally stop the recorder with R2 on the same row.
