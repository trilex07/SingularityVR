---
name: openxr-native
description: OpenXR and native Windows x86 integration guidance for SingularityVR's D3D9 proxy, session lifecycle, actions, poses, and frame submission.
metadata:
  source: project-specific
---

# OpenXR native integration for SingularityVR

Use for OpenXR loader/session/actions/reference spaces, controller poses, stereo views, or frame lifecycle work in this native x86 D3D9 project.

- Read `ENVIRONMENT.md`, `ENGINE_NOTES.md`, and the actual call path in `spikes/view_matrix/d3d9.cpp` before changing API use. Set `OPENXR_SDK` to the proven local SDK; do not install or fetch another runtime/SDK as a shortcut.
- Follow OpenXR's current official specification/SDK docs for unfamiliar enums, structure chains, action paths, and runtime behavior. Never infer action-path validity from names or copy Quest-specific assumptions into this Windows PCVR path.
- Keep session state transitions, focus, swapchain ownership, and teardown paired. Treat `xrWaitFrame`, `xrBeginFrame`, `xrEndFrame`, and action sync ordering as invariants and measure before optimizing.
- Separate mathematical/simulator checks from real Steam runtime verification. A valid OpenXR pose is not proof that the game rendered the corresponding hand, weapon, prompt, or eye correctly.
- Use `singularityvr-agent-test-bridge` before runtime-affecting changes and record the exact build/deployed hash.
