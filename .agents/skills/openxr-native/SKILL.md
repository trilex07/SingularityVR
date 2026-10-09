---
name: openxr-native
description: OpenXR and native Windows x86 integration guidance for SingularityVR's D3D9 proxy, session lifecycle, actions, poses, and frame submission.
metadata:
  source: project-specific
---

# OpenXR native integration for SingularityVR

Use for OpenXR loader/session/actions/reference spaces, controller poses, stereo views, 32-bit runtime selection, or frame lifecycle work in this native x86 D3D9 project. Use `ue3-d3d9-openxr` for engine/render-hook evidence and `vr-session-evidence` for recording/capture/replay.

- Read `ENVIRONMENT.md`, `ENGINE_NOTES.md`, and the actual call path in `spikes/view_matrix/d3d9.cpp` before changing API use. Set `OPENXR_SDK` to the proven local SDK; do not install or fetch another runtime/SDK as a shortcut.
- Follow OpenXR's current official specification/SDK docs for unfamiliar enums, structure chains, action paths, and runtime behavior. Never infer action-path validity from names or copy Quest-specific assumptions into this Windows PCVR path.
- Keep session state transitions, focus, swapchain ownership, and teardown paired. Treat `xrWaitFrame`, `xrBeginFrame`, `xrEndFrame`, and action sync ordering as invariants and measure before optimizing.
- Verify the runtime actually selected per process: PE32 (`0x014c`), manifest path, runtime name in the mod log, session state, and two projection views. Do not infer simulator success from a registry runtime or a created instance.
- Native OpenXR, a SteamVR compatibility bridge, and xrsim are separate backends. Never switch the machine-wide registry to select a test runtime; use a disposable manifest/environment route and restore it after the trial.
- Separate mathematical/simulator checks from real Steam runtime verification. A valid OpenXR pose is not proof that the game rendered the corresponding hand, weapon, prompt, or eye correctly.
- Use `singularityvr-agent-test-bridge` before runtime-affecting changes and record the exact build/deployed hash.
