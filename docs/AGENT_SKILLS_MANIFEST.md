# SingularityVR agent skills

Project-local skills are discovered from `.agents/skills/<name>/SKILL.md` by Codex/OpenCode-compatible Agent Skills loaders. OpenCode's current locations and metadata requirements are documented at <https://opencode.ai/docs/skills/>. Project rules are in [`AGENTS.md`](../AGENTS.md).

| Project skill | Origin | Source revision / note |
|---|---|---|
| `singularityvr-main-engineering` | SingularityVR project | Project-authored; MAIN evidence-driven debugging, safe takeover, complete payload/restore identities, Steam/PSVR2 tests and native asset/audio integration. |
| `singularityvr-weapons-continuation` | SingularityVR project | Compact weapons/Centurion handoff: role, project context, safety and separately dated current receipt. No Steam authority or runtime change. |
| `singularityvr-agent-test-bridge` | SingularityVR project | Project-authored; mandatory runtime evidence gate. |
| `vr-mod-workflow` | Adapted from `VR-Mod-Vibecoding-Wizard` | `bigbossafman/VR-Mod-Vibecoding-Wizard`, `40382b390e239f354ea3cecd1f9151c455883d7c`; local workflow is project-specific. |
| `native-game-modding` | `universal-modder/skills/mod-any-game` | `rehan-remade/universal-modder`, `8370faa8e114baf33acdb23079aff552a7728c4b`. |
| `game-automation` | `universal-modder/skills/game-automation` | `rehan-remade/universal-modder`, `8370faa8e114baf33acdb23079aff552a7728c4b`. |
| `game-recon` | `universal-modder/skills/game-recon` | `rehan-remade/universal-modder`, `8370faa8e114baf33acdb23079aff552a7728c4b`. |
| `mod-reverse-engineering` | `universal-modder/skills/reverse-engineering` | `rehan-remade/universal-modder`, `8370faa8e114baf33acdb23079aff552a7728c4b`. |
| `reverse-engineering` | `reverse-engineerskill/skills/reverse-engineering` | `juank115/reverse-engineerskill`, `02a29545cc16599b5f181f082a9d96dbce82791f`; read-only `scripts/triage.py` is included, not executed. |
| `openxr-native` | SingularityVR project | Project-authored native Windows x86/OpenXR guidance. |
| `xr-performance` | `awesome-gamedev-agent-skills/skills/disciplines/performance-optimization` | `gamedev-skills/awesome-gamedev-agent-skills`, `0a70cfc64672d512c5b5b6006b5dcb5cf90dbdbd`. |

The upstream source checkouts and detailed revision record are maintained outside this production worktree under `agent-skills-sources/`; they are not runtime dependencies. The source safety review is copied into [`AGENT_SKILLS_SECURITY_REVIEW.md`](AGENT_SKILLS_SECURITY_REVIEW.md) so future agents can review the installed set without that staging directory.

Validation on 2026-10-13: all ten project skill directories have a `SKILL.md`; every frontmatter `name` matches its directory name and every skill has a non-empty `description`. OpenCode CLI is not installed here, so CLI-level discovery has not been exercised; restart/reload a Codex/OpenCode session after changing skill metadata.

**MAIN difficult bugs:** read `singularityvr-main-engineering` after `AGENTS.md` and `singularityvr-agent-test-bridge`; use `vr-mod-workflow` for feature gates. Case studies live under `.agents/skills/singularityvr-main-engineering/references/`.

**MAIN takeover/promotion:** the same skill now routes to
[`continuity-and-asset-integration.md`](../.agents/skills/singularityvr-main-engineering/references/continuity-and-asset-integration.md).
Use it before reusing another agent's test runner or deploying DLLs with dependent
fonts/localization/audio. It stores the reusable method, not changing live hashes;
read current task receipts for installation facts. No additional runtime dependency
or Steam permission is introduced.
