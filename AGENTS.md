# SingularityVR project rules

These rules apply to all work in this repository and override generic skill defaults.

## Production safety

- Check the current branch, HEAD, worktree list, and full diff before editing. Preserve every existing change; never reset, force-push, rewrite history, blindly stash, or overwrite another worktree's files.
- The game is Singularity (UE3, D3D9, x86). Build production DLLs as x86 PE32 with `/O2`, using the proven OpenXR SDK at `C:\Users\Trilex\vr-mods\dependencies\openxr-local` via `OPENXR_SDK`.
- Before any Steam runtime test, require the built DLL SHA-256 to equal `Singularity\Binaries\d3d9.dll`; also verify PE32/x86 and record branch/commit. Do not test an artifact with uncertain provenance.
- Use `tools/singularityvr-test-bridge.ps1` for every change affecting runtime, OpenXR, rendering, stereo, hands, weapons, input, VFX, shadows, UI, menus, performance, or deployment. Its `Preflight` gate is required before launch; collect logs/captures and a manifest afterward.
- Never deploy over a running game. Back up install files through the project's deployment tooling. Do not modify saves, Steam/OpenXR registry values, or install external dependencies unless separately authorized.
- Do not guess engine addresses, offsets, register ownership, or pass identity. Require source evidence plus a falsification path; use one runtime variable at a time when debugging.
- Keep diagnostics bounded: no per-draw allocations, filesystem writes, or unbounded logs. Aggregate counters and cap captures.
- Review skill-bundled helper scripts before execution. Prefer read-only checks on project-controlled artifacts; do not run helpers on user saves or unknown binaries without explicit authorization.
- Every side agent must work in its own worktree and sandbox/data directory. Side agents must not deploy or replace DLLs in Steam, alter the OpenXR registry, or reuse another agent's xrsim/log/capture directory; only the primary agent owns Steam deployment.

## Evidence gates

- Track features as `UNKNOWN`, `BROKEN`, `IMPLEMENTED_NOT_VERIFIED`, `L1_PASS`, `L2_PASS`, `L3_PASS`, `READY_FOR_PHYSICAL_ACCEPTANCE`, `PHYSICALLY_VERIFIED`, or `BLOCKED` in `docs/VR_STATUS.md`.
- L1 covers deterministic math/synthetic checks; L2 covers build, tests, PE32, `/O2`, provenance and SHA; L3 uses the installed Steam game and the verified deployed DLL; L4 is the physical headset pass after autonomous gates pass.
- A compile, counter, initialization line, deployment, simulator run, or plausible code path cannot establish real runtime behavior by itself. Do not call a feature fixed without the evidence required by its gate.
- Use the test bridge and `vr-mod-workflow` skill for runtime work. Do not make the user the first-line debugger. Reserve a short, concrete headset script for perceptual questions that cannot be answered autonomously.

## Project context

- Read `docs/VR_STATUS.md` and `docs/VR_FEATURE_GAP_ANALYSIS.md` first. For engine-specific work read `ENGINE_NOTES.md`; search `HISTORY.md` for the evidence behind inherited claims rather than reading it cold.
- Prefer fixes for P0 lifecycle/performance, P1 stereo/render correctness, P2 hands/weapons, and P3 contextual interaction before optional polish.
- Protect the user's existing working tree. Make focused changes and report exact evidence and remaining limits.

## Skill routing

Project-local skills are in `.agents/skills/<name>/SKILL.md`, which is the repository-local Agent Skills location detected by Codex and OpenCode. Use the narrowest matching skill automatically:

- `ue3-d3d9-openxr` for UE3/D3D9 reverse engineering, stereo, shaders, shadows, VFX, per-eye state, and x86 OpenXR invariants.
- `openxr-native` for loader/session/actions/reference spaces and 32-bit runtime selection.
- `vr-session-evidence` for xrsim, `.vrrec`, per-eye capture, replay, frame pacing, and evidence manifests.
- `singularityvr-agent-test-bridge` for every runtime-affecting change and all L1–L4 evidence gates.
- `mod-reverse-engineering` for static/dynamic engine analysis; `game-automation` for controlled game driving; `xr-performance` for frame-budget investigations; `vr-mod-workflow` for staged VR implementation and regression planning.

Do not call a feature `FIXED` unless the required L1/L2/L3 evidence exists; L4 is required only for claims that cannot be proven autonomously. The primary agent must use the test bridge without waiting for an additional prompt.
