# Foreground boundary guard — 2026-10-08

The renderer used a late depth-buffer clear as the start of the first-person
pass. Shadow and reflection passes can issue the same clear, so the old flag
could stay set for thousands of world draws. That made the widened first-person
effect path repeatedly enter its bail branch, adding frame cost and leaving
muzzle effects at the engine camera instead of on the controller-ridden gun.

The current path now:

- confirms the boundary after a known gun/arm vertex signature appears;
- allows only the measured short pass (or the first eight probe draws while the
  signature is being recognised);
- rejects the rest of a false shadow/reflection span;
- counts the bail once per frame instead of once per world draw;
- resets the confirmation at every `Present` boundary.

Independent left-hand palette preparation is also allowed when the whole-mesh
ride mode is off. It still requires the exact single-player section/range,
valid off-hand tracking, the shader CTAB layout, and the foreground window.

Validation:

- `test-foreground-boundary.exe` — pass;
- `test-vfx-eye-scale.exe` — pass;
- `test-vr-interaction.exe` — pass;
- `test-vr-transition-sim.exe` — pass;
- `test-production-runtime.exe` — pass;
- `test-reference-integration.exe` — pass;
- `test-parallel-components.exe` — pass;
- `test-manual-reload.exe` — pass;
- x86 `/O2` DLL build — PE32 machine `0x14C`;
- dvr-xrsim OpenXR selftest — 60 focused frames, zero errors.

The emulator screenshot is stored at
`deployment/visual-audit-20261008/foreground-boundary-emulation.png`. The
middle and right panels are deterministic desktop/OpenXR emulation; a new
physical-headset SBS capture is still needed to visually confirm the real
shadows and barrel position.
