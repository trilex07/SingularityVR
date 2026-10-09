# Steam smoke test — 2026-10-09

The final deployed build starts through Steam and reaches the D3D9Ex/OpenXR
path without a DirectInput mouse:

```
Direct3DCreate9Ex succeeded
startup DirectInput system mouse: hr=0x80040154 device=00000000
OpenXR ready
ini: Upscaling Mode=Off ...
```

The test machine's final run did **not** have an active OpenXR controller
profile. The mod recorded:

```
interaction profile [left hand]: (none - runtime bound nothing)
interaction profile [right hand]: (none - runtime bound nothing)
physical XR inputs: ... action 0 pose 0 position 0 ... UI 0
controllers [MOTION FALLBACK - no touch bindings]
```

SteamVR itself did load the app's Oculus Touch binding and reported successful
automatic remapping to `playstation_vr2_sense`, but no live hand profile reached
the game process in this run. This is a controller/runtime state, so this smoke
test cannot certify R2 menu input or the visible VR cursor. A physical run with
both Sense controllers awake and tracking is still required for that check.
