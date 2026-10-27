# Agent workflows

Project rules live in [`AGENTS.md`](../AGENTS.md). Codex and OpenCode project skills
are stored in `.agents/skills/<name>/SKILL.md`; project rules take precedence over
generic skill defaults.

| Skill | Use for |
|---|---|
| `singularityvr-agent-test-bridge` | Mandatory evidence gates and Steam/xrsim runtime workflow for all runtime-affecting changes. |
| `vr-mod-workflow` | Implementing and stabilizing an existing SingularityVR feature. |
| `vr-mod-vibecoding-wizard` | Starting or triaging a complete flatscreen-to-VR conversion; on this repo, resume from the existing status docs and skip answered setup questions. |
| `openxr-native` | OpenXR lifecycle, actions, poses, stereo views, and native Windows x86 integration. |
| `native-game-modding` | Choosing safe approaches for native game modifications. |
| `game-automation` | Planning repeatable real-game automation and evidence capture. |
| `game-recon` | Reconnaissance before modifying a new game. |
| `mod-reverse-engineering` | Game-specific native reverse engineering. |
| `reverse-engineering` | General Windows PE/x86 reverse engineering. |
| `xr-performance` | Measurement-led VR profiling and frame-budget analysis. |

The matching `SKILL.md` files and their upstream licenses/source metadata are kept
beside this index. Do not run bundled helper scripts before reviewing them.

## Discovery verification

`.agents/skills/<name>/SKILL.md` is the repo-local skill location documented for
Codex and is also an OpenCode-compatible project location. OpenCode's discovery
rules include `.agents/skills` and project `AGENTS.md` files:
[OpenCode skills](https://docs.opencode.ai/docs/skills/),
[OpenCode rules](https://docs.opencode.ai/docs/rules/). OpenAI documents repo-local
skills under `.agents/skills` for Codex:
[OpenAI's Codex OSS workflow](https://developers.openai.com/blog/skills-agents-sdk).

The skills in this repository have matching lowercase directory/`name` values
and `description` front matter. The current machine has no `opencode` executable
or project `opencode.json`; therefore path compatibility is verified against
the official discovery rules, but live OpenCode CLI enumeration has not been
verified here. No duplicate `.opencode/skills` copy is maintained because it
would shadow the canonical shared definitions.
