# Agent setup

SingularityVR keeps project instructions and task-specific skills in the repository so both Codex and OpenCode can discover them without a user-pasted prompt.

## Project instructions

`AGENTS.md` is the concise, authoritative project rule set. MAIN owns Steam deployment/integration testing; side agents work in isolated worktrees and verify with L1/L2 only. Keep procedures in skills and current evidence in `STATUS.md`. (`docs/VR_STATUS.md` and `docs/VR_FEATURE_GAP_ANALYSIS.md` do not exist in this tree.)

## Project skills

The project skills live in `.agents/skills/<skill-name>/SKILL.md`. OpenCode discovers this Agent Skills-compatible project location; see <https://opencode.ai/docs/skills/>. The project includes `singularityvr-agent-test-bridge`, `vr-mod-workflow`, `native-game-modding`, `game-automation`, `game-recon`, `mod-reverse-engineering`, `reverse-engineering`, `openxr-native`, and `xr-performance`. Portable origin/revision metadata and local frontmatter validation are in [`AGENT_SKILLS_MANIFEST.md`](AGENT_SKILLS_MANIFEST.md); the safety review is in [`AGENT_SKILLS_SECURITY_REVIEW.md`](AGENT_SKILLS_SECURITY_REVIEW.md). The upstream checkouts and historical install artifacts remain in the sibling `agent-skills-sources/` staging directory and are not required to discover or use the project skills.

After changing these files, start a new Codex task or reload project context before expecting changed skill instructions to be available to the agent. OpenCode's CLI is not installed in the current environment, so CLI-level discovery cannot be invoked here; the repository layout and skill metadata are validated locally.

## Steam and runtime testing (MAIN only)

Side agents never touch the Steam installation. MAIN owns all Steam deployment and integration testing from its own worktree using the checked-in `tools/deploy-singularityvr.ps1` and `tools/steam-xrsim-singularity.ps1`. (`tools/singularityvr-test-bridge.ps1` does not exist in this tree; do not treat it as a required gate.) Each agent works in its own isolated worktree/branch, commits and pushes only its own branch, and opens a PR for review without merging. Read the MAIN worktree's `.agents/skills/singularityvr-agent-test-bridge/SKILL.md` before changing runtime evidence procedures.

## Refreshing upstream skill copies

Review the upstream source and revision first. Preserve its license and attribution. Copy only the required skill directory into `.agents/skills`, retaining upstream identity in metadata where the local folder name differs. Never run an upstream installer script as part of refresh.
