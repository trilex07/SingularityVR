---
name: ue3-d3d9-openxr
description: Reverse-engineer and validate UE3/D3D9 x86 VR hooks, stereo passes, shaders, shadows, VFX, and OpenXR frame ownership without guessing offsets or declaring FIXED without L1-L4 evidence.
license: MIT
compatibility: codex, opencode
metadata:
  project: SingularityVR
  source: adapted-from-vr-stereo-hub-and-universal-modder
  target: UE3-D3D9-x86
---

# UE3/D3D9/OpenXR hook workflow

Use this skill for engine internals, D3D9 render interception, per-eye state, shader/VFX or shadow work, and x86 OpenXR integration. It is a project procedure, not a license to import addresses from another game.

## Non-negotiable setup

1. Read `AGENTS.md`, `docs/VR_STATUS.md`, `docs/VR_FEATURE_GAP_ANALYSIS.md`, `ENGINE_NOTES.md`, and the relevant evidence note before editing.
2. Work in the agent's own worktree/sandbox. Never deploy a DLL, edit Steam files, or touch `d3d9.cpp` in a skills/research task.
3. Record an observed failure, a falsifiable hypothesis, the source evidence, and the smallest probe before changing code.
4. Treat every address, vtable slot, render-pass classification, and matrix layout as `UNKNOWN` until it is proven on the exact Singularity build.

## UE3/D3D9 investigation loop

- Establish the exact executable, PE32/x86 identity, build/version, and module list.
- Prefer symbols, signatures, call-site evidence, and bounded traces over hard-coded offsets. A signature must have uniqueness and a negative/control case.
- Trace the full path: game camera or render hook → view/projection state → per-eye transaction → shader/texture/RT state → Present/compositor submission.
- Classify main color, HUD/UI, shadow/depth, reflection, post-process, and VFX passes separately. Do not apply the main-eye IPD delta to light-space or shared resources.
- For shader changes, capture the input CTAB/layout and restore every modified D3D9 state at the pair/eye boundary. Keep per-draw diagnostics bounded.
- For hands/IK, prove coordinate-space and unit conversion, then use the existing game skeleton/animation where possible; animation takeover and controller return must be explicit state transitions.

## OpenXR/x86 invariants

- The game and injected loader path are 32-bit. Verify PE machine `0x014c` before any runtime claim; a silent fallback to the registry runtime invalidates the test.
- Keep `xrWaitFrame → xrBeginFrame → locate/views → render pair → xrEndFrame` aligned with the game's actual frame cadence. Separate once-per-frame work from per-eye work.
- Select a simulator per process through an explicit manifest/environment route; never change the machine-wide OpenXR registry as a test shortcut.
- Assert the runtime name from the mod log and assert session state plus two projection views. A created session or valid pose is not proof of stereo output.
- Native 32-bit OpenXR, SteamVR bridge, and simulator are distinct backends. Record which one actually ran.

## Evidence ladder

- L1: math, unit, synthetic, shader-policy, and replay tests.
- L2: x86 PE32 `/O2` build, provenance, and SHA-256 verification.
- L3: autonomous sandbox/xrsim, per-eye capture, test bridge, then Steam smoke test using only the verified artifact. Only the primary agent may replace a Steam DLL.
- L4: physical PSVR2 only for questions the autonomous evidence cannot prove.

Use exact status words from `docs/VR_STATUS.md`. `IMPLEMENTED_NOT_VERIFIED` is not `FIXED`; `FIXED` requires the appropriate gate evidence and a reproducible artifact.

## Adapted lessons and limits

BioShock VR demonstrates a separate 32-bit simulated runtime, runtime-name assertions, PE guards, per-eye compositor captures, and a square render-target budget. Dishonored VR demonstrates the same simulator discipline plus an explicit eye-check with pairing, ocular difference, head-motion difference, and focus guards. These are procedures and design lessons only: do not copy their DLLs, offsets, engine assumptions, or runtime binaries into SingularityVR.
