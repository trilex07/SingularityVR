# Agent skill source safety review

Reviewed repository file inventories and the selected `SKILL.md` trees before installation.

- No third-party installer, shell script, package manager command, binary download, or runtime hook was executed.
- Installer-like files were deliberately not run. Examples include scripts in `claude-reverse-skills`, `reverse-engineerskill`, `xr-dev`, and APK-reverse cleanup helpers that use `rm -rf`.
- The selected skills are documentation/references, plus one read-only triage helper in the general reverse-engineering source. Its source was reviewed; it was not invoked as part of skill installation.
- Meta Quest/Horizon/Android-specific skills were excluded because they introduce platform assumptions that do not apply to this Windows x86 UE3/D3D9/OpenXR project.
- Project policy in [`AGENTS.md`](../AGENTS.md) overrides generic skill instructions. In particular, do not install external tools/dependencies or execute a skill helper without separate authorization and review.
- Future helper scripts must be reviewed and run only against disposable copies or explicitly approved test artifacts.
