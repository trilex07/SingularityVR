# VR session recorder

The production DLL now exposes `VR Settings -> CONTROLLER -> RECORD SESSION`.
R2 over the row starts or stops a session; the L3+R3 chord never changes this
state. The chord no longer takes screenshots and optionally recentres according
to `[Input] RecenterChord=1`.

Recordings are written to:

`%LOCALAPPDATA%\SingularityVR\vr_sessions\session_YYYY-MM-DD_HH-MM-SS.vrrec`

The stream has a small fixed header followed by packed 90 Hz input samples. A
sample includes HMD and both controller poses, triggers, sticks, UI coordinates,
menu/panel/gameplay flags, weapon/holster/two-hand state, prompt state, muzzle
position, and fire/melee/recenter event bits. While disabled there is no file I/O
or per-frame allocation. While enabled the DLL batches 64 samples before each
write so the input path does not open or append a file every frame.

`tools/vr_session_dump.py` validates the header and can export CSV:

```text
python tools/vr_session_dump.py session.vrrec session.csv
```

This is the capture/review half of replay support. A physical PSVR2 run is
still needed once to collect a real route; afterwards the CSV/binary stream can
drive deterministic desktop checks for state transitions and pointer/weapon
events without claiming optical headset validation.
