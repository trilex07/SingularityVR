---
name: singularityvr-agent-test-bridge
description: Mandatory evidence workflow for SingularityVR runtime, OpenXR, rendering, input, hands, VFX, shadows, menus, performance, and deployment changes.
---

# SingularityVR agent test bridge

Use this skill for every change that can affect the running mod. Start at the production worktree and preserve its existing diff.

1. Read `AGENTS.md`, `docs/VR_STATUS.md`, the target feature's evidence, and the current `d3d9.cpp` path.
2. Run `tools/singularityvr-test-bridge.ps1 -Action Preflight`. Require the source-built DLL and installed Steam DLL to have identical SHA-256, and require x86 PE32. Record branch and commit. If the build flags cannot be proven to include `/O2`, rebuild before runtime testing.
3. Run the narrowest relevant deterministic test/replay first. Preserve source recordings; use `vr_session_dump.py` and `vr_session_replay.py` offline. For an installed-game simulated OpenXR route, use `steam-xrsim-singularity.ps1` with an explicit `-CandidateDirectory`, `-LaunchViaSteam`, and `-RestartSteam`; pass `-LogRoot` to capture the user's actual runtime directory. Replay `.vrrec` only after the xrsim state reports `FOCUSED` and two projection views.
4. For a physical SteamVR route, use `tools/singularityvr-test-bridge.ps1 -Action Launch` only after preflight passes. Keep runtime experiments to one variable. Do not deploy while the game is running. For menu/desktop inspection, launch visibly in a window when the route supports it and use the environment's approved UI automation surface to inspect/control the actual window; never guess a menu action. A window size that changes the stereo scene/backbuffer match disables stereo in this build, so windowed UI checks do not prove rendering, hands, or stereo. If the surface cannot enumerate the running game, preserve the runtime evidence and state the UI boundary. An xrsim pass proves installed-game integration, not physical headset behavior.
5. Collect evidence with `tools/singularityvr-test-bridge.ps1 -Action Collect -LogRoot <runtime-log-dir> -XrSimDirectory <xrsim-dir>`. Keep bounded captures, current log, xrsim state/log, process/build provenance, and a manifest together. Do not collect whole log directories or create per-draw output.
6. If the Windows ACL blocks a candidate write, rerun the same reviewed project runner with elevated permissions; retain its timestamped backup and verify its automatic restore. Do not copy directly into the Steam directory.
7. Compare against the same scene/recording, report L1/L2/L3 separately, and update `docs/VR_STATUS.md`. Mark physical acceptance ready only after autonomous tests pass; never mark physically verified without an actual headset result.

The bridge is read-only for preflight and collection. Steam launch is an explicit separate action and must not replace the user's deployed DLL or INI.
