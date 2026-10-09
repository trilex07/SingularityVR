# Agent skill source safety review

Reviewed repository file inventories and the selected `SKILL.md` trees before installation.

- No third-party installer, shell script, package manager command, binary download, or runtime hook was executed.
- Installer-like files were deliberately not run. Examples include scripts in `claude-reverse-skills`, `reverse-engineerskill`, `xr-dev`, and APK-reverse cleanup helpers that use `rm -rf`.
- The selected skills are documentation/references, plus one read-only triage helper in the general reverse-engineering source. Its source was reviewed; it was not invoked as part of skill installation.
- Meta Quest/Horizon/Android-specific skills were excluded because they introduce platform assumptions that do not apply to this Windows x86 UE3/D3D9/OpenXR project.
- Project policy in [`AGENTS.md`](../AGENTS.md) overrides generic skill instructions. In particular, do not install external tools/dependencies or execute a skill helper without separate authorization and review.
- Future helper scripts must be reviewed and run only against disposable copies or explicitly approved test artifacts.
- BioShock's upstream `bvr_xrsim32.dll` and `xrsim-launch.ps1` were checked by source/documentation review. The DLL is not present in this repository and was not copied or executed. SingularityVR's existing simulator/replay tools were treated as project-controlled but no Steam launch or DLL deployment was performed in this branch.
- Dishonored's `dvr_xrsim32.dll`, `xrsim-launch.ps1`, and per-eye `eye-check.ps1` were inspected from the local checkout at commit `7c1cb8a323e889220d33e62dbb7ed2b64ec765a5` and compared with upstream branch tip `b15e4d32a720bd4cfdb5ce4d600f3f8fba8d2cec`; no launcher, runtime, or binary was imported.
