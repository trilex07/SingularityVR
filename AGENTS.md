# SingularityVR project rules

These rules apply to all work in this repository and override generic skill defaults.

## Agent workflow (authoritative)

- MAIN owns the Steam installation, Steam deployment, and Steam integration testing. Only MAIN runs `tools/deploy-singularityvr.ps1`, `tools/steam-xrsim-singularity.ps1`, or any launch/deploy against the installed Steam game.
- Side agents work in their own isolated worktree/branch and never touch Steam: no Steam launches, no Steam deploys, no restore/rollback of the Steam install, no `steam-xrsim` runs against the user's game. Side agents verify with L1 (deterministic math/synthetic checks) and L2 (x86 PE32 `/O2` build, tests, provenance/SHA) only.
- No agent overwrites another agent's changes: never edit files in another worktree, never commit/push to another agent's branch, never reset/force-push/rewrite history. Check `git worktree list`, branch, HEAD, and full diff before editing.
- Isolated work: one branch per worktree (`git worktree add -b <branch> <path> origin/main`). Commit only your own worktree's files, push only your own branch, open a PR for review, do not merge your own PR.
- Physical PSVR2 verification is separate from automatic tests: L1/L2 and optional xrsim evidence never count as headset acceptance. Never mark `PHYSICALLY_VERIFIED` without an actual headset result; mark `READY_FOR_PHYSICAL_ACCEPTANCE` only after autonomous gates pass.
- The xrsim/Steam runner stages are optional integration evidence owned by MAIN, not mandatory gates for side agents. Do not require missing tools or simulator passes to land L1/L2 work.

## Production safety

- Check the current branch, HEAD, worktree list, and full diff before editing. Preserve every existing change; never reset, force-push, rewrite history, blindly stash, or overwrite another worktree's files.
- The game is Singularity (UE3, D3D9, x86). Build production DLLs as x86 PE32 with `/O2`, resolving the OpenXR SDK via `OPENXR_SDK` and `tools/Resolve-OpenXrSdk.ps1` (never hardcode a machine-specific SDK path in committed files).
- Before any MAIN-owned Steam runtime test, require the built DLL SHA-256 to equal `Singularity\Binaries\d3d9.dll`; also verify PE32/x86 and record branch/commit. Do not test an artifact with uncertain provenance.
- Side agents do not run runtime bridges against Steam. MAIN uses the checked-in `tools/steam-xrsim-singularity.ps1` for repeatable installed-game launches (see `docs/STEAM_XRSIM_RUNNER_PLAYBOOK.md`) and `tools/deploy-singularityvr.ps1` for deployment; `tools/singularityvr-test-bridge.ps1` does not exist in this tree, so do not reference it as a required gate. Collect logs/captures and a manifest afterward.
- For MAIN-owned tests that need menu or desktop inspection, MAIN may launch a visible windowed game where the route permits and inspect/control it through the environment's approved UI automation surface; never guess UI selection. A window size that differs from the game's stereo scene/backbuffer can disable stereo, so windowed UI checks never count as stereo, hand-rendering, or visual acceptance. If no approved surface can expose a live game window, record that exact boundary and continue with independent runtime evidence. Side agents do not launch the game.
- If Windows ACLs block deployment, run the existing project runner with elevated privileges; keep its preflight, timestamped backup, and automatic restore. Never hand-copy over the installed DLL or INI.
- Never deploy over a running game. Back up install files through the project's deployment tooling. Do not modify saves, Steam/OpenXR registry values, or install external dependencies unless separately authorized.
- Do not guess engine addresses, offsets, register ownership, or pass identity. Require source evidence plus a falsification path; use one runtime variable at a time when debugging.
- Keep diagnostics bounded: no per-draw allocations, filesystem writes, or unbounded logs. Aggregate counters and cap captures.
- Review skill-bundled helper scripts before execution. Prefer read-only checks on project-controlled artifacts; do not run helpers on user saves or unknown binaries without explicit authorization.

## Evidence gates

- Track features as `UNKNOWN`, `BROKEN`, `IMPLEMENTED_NOT_VERIFIED`, `L1_PASS`, `L2_PASS`, `L3_PASS`, `READY_FOR_PHYSICAL_ACCEPTANCE`, `PHYSICALLY_VERIFIED`, or `BLOCKED` in `STATUS.md`.
- L1 covers deterministic math/synthetic checks; L2 covers build, tests, PE32, `/O2`, provenance and SHA; L3 is MAIN-only and uses the installed Steam game and the verified deployed DLL; L4 is the separate physical headset pass after autonomous gates pass.
- A compile, counter, initialization line, deployment, simulator run, or plausible code path cannot establish real runtime behavior by itself. Do not call a feature fixed without the evidence required by its gate.
- Use the `vr-mod-workflow` skill for runtime work. Do not make the user the first-line debugger. Reserve a short, concrete headset script for perceptual questions that cannot be answered autonomously.
- MAIN-owned L3 defaults to a direct real-Steam test (build, verify SHA/PE32, backup, deploy via `tools/deploy-singularityvr.ps1`, test, rollback if needed); do not hand-launch `Singularity.exe` or assume an already-running Steam client inherited the XR runtime. The `tools/steam-xrsim-singularity.ps1` xrsim variant (see `docs/STEAM_XRSIM_RUNNER_PLAYBOOK.md`) is explicitly optional integration evidence, never a mandatory gate. Side agents run neither.
- The deploy/runner tooling is a controlled deployment aid, not an automatic production deployer: MAIN reviews the candidate SHA, lets preflight create the backup, and uses `-NoRestore`/`-KeepCandidate` only as an explicit main-agent decision. Never run it against another agent's process or worktree.

## Project context

- Read `STATUS.md` first. For engine-specific work read `ENGINE_NOTES.md`; search `HISTORY.md` for the evidence behind inherited claims rather than reading it cold. (`docs/VR_STATUS.md` and `docs/VR_FEATURE_GAP_ANALYSIS.md` do not exist in this tree; do not reference them as required reading.)
- Repo-local Codex/OpenCode skills live under `.agents/skills/<name>/SKILL.md`; keep their required `name` and `description` front matter and preserve source licenses. See `docs/AGENT_SKILLS_MANIFEST.md` for the workflow index. (`docs/AGENT_SKILLS.md` does not exist; the manifest is the index.)
- Prefer fixes for P0 lifecycle/performance, P1 stereo/render correctness, P2 hands/weapons, and P3 contextual interaction before optional polish.
- Protect the user's existing working tree. Make focused changes and report exact evidence and remaining limits.


# Singularity VR — Evidence-Driven Debugging & Reverse Engineering Policy

This policy applies to ALL MAIN and SIDE agents working on difficult, repeatedly unresolved technical problems.

## Escalation Rule

After two unsuccessful implementations, or when previous research repeatedly fails to explain the observed behavior, STOP speculative patching.

Switch to evidence-driven reverse engineering.

Do not keep changing shader constants, offsets, rendering routes or engine behavior without identifying the actual producer and mechanism.

## Required Workflow

### 1. Observe the actual game behavior

Identify what the game really does, not what you expect it to do.

Use appropriate techniques:
- Targeted D3D9/OpenXR instrumentation.
- UE3 UObject/class/property reflection.
- Shader, texture and render-target tracking.
- Input and frame-timing traces.
- Native assembly/disassembly and Ghidra.
- Scaleform/GFx asset extraction and ActionScript analysis.
- Controlled binary and memory inspection.
- Comparisons against real working VR mods.

Prefer observation through existing hooks over installing new invasive hooks.

### 2. Prove the relevant execution path

Identify the responsible function, shader, render target, resource, matrix, input handler or engine object.

Record actual runtime evidence:
- Object and resource identity.
- Relevant state changes.
- Call timing and ordering.
- Data written and consumed.
- Reproducible behavior.
- Evidence that distinguishes the candidate cause from unrelated operations.

A counter increment, object name, successful compilation or valid pointer is not automatically proof of correct behavior.

### 3. Form a falsifiable hypothesis

Before modifying code, state:

- What exact mechanism is believed to be broken?
- What evidence supports it?
- What observation would disprove it?
- What is the smallest test capable of distinguishing the possibilities?

Do not guess shader registers, UE3 offsets or GFx variable paths.

### 4. Implement the smallest controlled experiment

Prefer:
- Read-only probes.
- Bounded diagnostic logs.
- Captured resource identities.
- Pixel/alpha readback where relevant.
- Small offline reproductions.
- Isolated default-OFF experimental features.

Preserve D3D9 state, COM lifetime and threading semantics. Never allow diagnostic code itself to corrupt the rendering pipeline.

### 5. Test, analyze and iterate

Use:

OBSERVE → HYPOTHESIZE → INSTRUMENT → MEASURE → IMPLEMENT → TEST → REFINE.

Do not stop after rollback or after producing another report.

If an experiment fails, use its actual evidence to determine the next justified experiment.

Do not repeat disproven approaches without new supporting evidence.

### 6. Distinguish evidence levels

Always distinguish:

- Offline structural PASS.
- Compile/test PASS.
- Real Steam runtime PASS.
- OpenXR session confirmed.
- Pixel/content validation PASS.
- Physical PSVR2 visual PASS.

Never claim higher-level validation from lower-level evidence.

### 7. Preserve the accepted build

Accepted systems are frozen unless explicitly included in the active task.

Only MAIN may:
- Deploy into Steam.
- Modify installed DLL/config/assets.
- Run controlled Steam integration tests.

SIDE agents use isolated branches/worktrees and perform offline research and tests.

MAIN must back up original files, verify exact hashes and restore the accepted installation after unsuccessful experiments.

Never claim rollback succeeded without verifying the actual restored files.

### 8. Ask for physical VR verification when necessary

If automation cannot verify the required visual behavior, MAIN must prepare a safe candidate and ask the user for a short PSVR2 test.

Do not silently abandon the experiment just because the game window cannot be observed automatically.

Do not repeatedly ask the user to verify unchanged baseline behavior.

### 9. Continue toward a working solution

A failed experiment is a diagnostic result, not task completion.

Continue autonomously while safe, productive experiments remain.

Stop and report a blocker only when further work requires unavailable evidence, a risky or irreversible action, or explicit user authorization.

## Git Rules

Each agent must:
- Work on a dedicated branch/worktree.
- Commit meaningful changes.
- Push completed work.
- Document key findings without excessive reporting.
- Never merge into main without authorization.

## Primary Principle

**Understand the system before changing it.**

For difficult bugs, concrete reverse-engineering evidence is more valuable than another speculative patch.

The objective is a working, verified solution — not merely a successfully built candidate or a restored baseline.