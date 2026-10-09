# VR runtime fixes — 2026-10-09

The current production build keeps the recorder and the recenter chord on
separate controls:

- `VR Settings -> CONTROLLER -> RECENTER L3+R3` enables or disables the chord.
- `VR Settings -> CONTROLLER -> RECORD SESSION` uses R2 to start/stop a file.
- L3+R3 never toggles recording. It only emits a recenter event when enabled
  and keeps the existing diagnostic screenshot request.

The gameplay context prompt overlay now advertises **R1** (right grip), which
matches the runtime use/reload action. Enter/Esc labels in the Scaleform text
path continue to map to Cross/Circle for notes and menus.

Full arm deformation is still opt-in. The shipped default is
`ShowControllerArms=0`, so the stable tracked gloves remain visible while the
animated arm mesh is not drawn. The VR panel can re-enable the arm mesh for a
comparison run.

The Steam deployment keeps a rollback DLL and INI beside the staged build.
The deployed DLL is x86 PE32 (`machine 0x14C`, optional-header magic `0x10B`).
Optical shadow and muzzle placement still require one physical PSVR2 pass;
the desktop tests cannot prove those pixels.
