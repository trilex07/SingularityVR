# SingularityVR agent skills

Project-local skills are discovered from `.agents/skills/<name>/SKILL.md` by Codex/OpenCode-compatible Agent Skills loaders. OpenCode's current locations and metadata requirements are documented at <https://opencode.ai/docs/skills/>. Project rules are in [`AGENTS.md`](../AGENTS.md).

| Project skill | Origin | Source revision / note |
|---|---|---|
| `singularityvr-agent-test-bridge` | SingularityVR project | Project-authored; mandatory runtime evidence gate. |
| `vr-mod-workflow` | Adapted from `VR-Mod-Vibecoding-Wizard` | `bigbossafman/VR-Mod-Vibecoding-Wizard`, `40382b390e239f354ea3cecd1f9151c455883d7c`; local workflow is project-specific. |
| `native-game-modding` | `universal-modder/skills/mod-any-game` | `rehan-remade/universal-modder`, `8370faa8e114baf33acdb23079aff552a7728c4b`. |
| `game-automation` | `universal-modder/skills/game-automation` | `rehan-remade/universal-modder`, `8370faa8e114baf33acdb23079aff552a7728c4b`. |
| `game-recon` | `universal-modder/skills/game-recon` | `rehan-remade/universal-modder`, `8370faa8e114baf33acdb23079aff552a7728c4b`. |
| `mod-reverse-engineering` | `universal-modder/skills/reverse-engineering` | `rehan-remade/universal-modder`, `8370faa8e114baf33acdb23079aff552a7728c4b`. |
| `reverse-engineering` | `reverse-engineerskill/skills/reverse-engineering` | `juank115/reverse-engineerskill`, `02a29545cc16599b5f181f082a9d96dbce82791f`; read-only `scripts/triage.py` is included, not executed. |
| `openxr-native` | SingularityVR project | Project-authored native Windows x86/OpenXR guidance. |
| `xr-performance` | `awesome-gamedev-agent-skills/skills/disciplines/performance-optimization` | `gamedev-skills/awesome-gamedev-agent-skills`, `0a70cfc64672d512c5b5b6006b5dcb5cf90dbdbd`. |
| `ue3-d3d9-openxr` | Adapted procedure | BioShock Trilogy VR `41f004bc6a1e4fa8be0d0e0749f9ac5a0453e75`, Dishonored VR `b15e4d32a720bd4cfdb5ce4d600f3f8fba8d2cec`, universal-modder `8370faa8e114baf33acdb23079aff552a7728c4b`; no foreign code or binaries copied. |
| `vr-session-evidence` | Adapted procedure | BioShock Trilogy VR `41f004bc6a1e4fa8be0d0e0749f9ac5a0453e75`, Dishonored VR `b15e4d32a720bd4cfdb5ce4d600f3f8fba8d2cec`; aligned to existing SingularityVR `.vrrec` and test bridge. |

The upstream source checkouts and detailed revision record are maintained outside this production worktree under `agent-skills-sources/`; they are not runtime dependencies. The source safety review is copied into [`AGENT_SKILLS_SECURITY_REVIEW.md`](AGENT_SKILLS_SECURITY_REVIEW.md), with the adaptation and simulator verdict in [`VR_AGENT_SKILLS_SOURCE_REVIEW.md`](VR_AGENT_SKILLS_SOURCE_REVIEW.md), so future agents can review the installed set without that staging directory.

Validation on 2026-10-09: all eleven project skill directories have a `SKILL.md`; every frontmatter `name` matches its directory name and every skill has a non-empty `description`. The current Codex/OpenCode convention is project-local `.agents/skills/<name>/SKILL.md`; OpenCode also supports `.opencode/skills` and `.claude/skills`, but duplicating IDs would create precedence ambiguity. CLI-level discovery is recorded in `docs/AGENT_SETUP.md` and must be rechecked after a session reload.
