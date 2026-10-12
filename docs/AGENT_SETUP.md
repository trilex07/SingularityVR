# Agent setup

SingularityVR keeps project instructions and task-specific skills in the repository so both Codex and OpenCode can discover them without a user-pasted prompt.

## Project instructions

`AGENTS.md` is the concise, authoritative project rule set. Keep procedures in skills and current evidence in `docs/VR_STATUS.md` and `docs/VR_FEATURE_GAP_ANALYSIS.md`.

## Project skills

The project skills live in `.agents/skills/<skill-name>/SKILL.md`. This is the documented project skill location supported by Codex and OpenCode. The project includes `singularityvr-agent-test-bridge`, `vr-mod-workflow`, `native-game-modding`, `game-automation`, `game-recon`, `reverse-engineering`, `openxr-native`, and `xr-performance`. Upstream identity and source review are recorded in `agent-skills-sources/INSTALLED-SKILLS-MANIFEST.md` and `SECURITY-REVIEW.md`.

After changing these files, start a new Codex task or reload project context before expecting changed skill instructions to be available to the agent. OpenCode's CLI is not installed in the current environment, so runtime discovery cannot be invoked here; the repository layout and skill metadata are validated locally.

## Mandatory runtime bridge

Runtime work belongs in the sibling `SingularityVR-production` worktree. From that worktree, use `tools/singularityvr-test-bridge.ps1`: `Preflight` gates on artifact provenance and SHA equality, `Launch` starts the Steam VR route only after the gate passes, and `Collect` gathers bounded evidence. From the canonical root, the bridge path is `..\SingularityVR-production\tools\singularityvr-test-bridge.ps1`. Read the production worktree's `.agents/skills/singularityvr-agent-test-bridge/SKILL.md` before changing the bridge.

## Refreshing upstream skill copies

Review the upstream source and revision first. Preserve its license and attribution. Copy only the required skill directory into `.agents/skills`, retaining upstream identity in metadata where the local folder name differs. Never run an upstream installer script as part of refresh.
