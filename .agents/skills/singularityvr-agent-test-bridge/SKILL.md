---
name: singularityvr-agent-test-bridge
description: Mandatory evidence workflow for SingularityVR runtime, OpenXR, rendering, input, hands, VFX, shadows, menus, performance, and deployment changes.
---

# SingularityVR agent test bridge

`AGENTS.md` owns the workflow: MAIN owns Steam deployment/integration testing; side agents never touch Steam and verify with L1/L2 only.

1. Read `AGENTS.md`, `STATUS.md`, the target feature's evidence, and the current source path. (`docs/VR_STATUS.md` and `tools/singularityvr-test-bridge.ps1` do not exist in this tree; do not require them.)
2. Side agents: run the narrowest relevant deterministic check (L1) and the x86 PE32 `/O2` build plus tests with provenance/SHA recorded (L2). Do not launch or deploy the Steam game.
3. MAIN only: for an installed-game simulated OpenXR route, use the checked-in `tools/steam-xrsim-singularity.ps1` with an explicit `-CandidateDirectory`, `-LaunchViaSteam`, and `-RestartSteam`. Replay `.vrrec` only after the xrsim state reports `FOCUSED` and two projection views. For deployment/restore use `tools/deploy-singularityvr.ps1` with its timestamped backup; never deploy over a running game.
4. Keep runtime experiments to one variable. An xrsim pass proves installed-game integration, not physical headset behavior.
5. Keep bounded captures, current log, xrsim state/log, process/build provenance, and a manifest together. Do not collect whole log directories or create per-draw output.
6. Report L1/L2/L3 separately and update `STATUS.md`. Mark physical acceptance ready only after autonomous tests pass; never mark physically verified without an actual headset result.
