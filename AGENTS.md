# SingularityVR project rules

These rules apply to the canonical repository. Runtime work and production builds belong in the sibling `SingularityVR-production` worktree; its `AGENTS.md` adds the production-specific procedures.

- Preserve every existing worktree diff. Never reset, force-push, rewrite history, blindly stash, or overwrite another worktree's files.
- Singularity is UE3/D3D9/x86. Production builds must be x86 PE32 `/O2`, use `C:\Users\Trilex\vr-mods\dependencies\openxr-local` through `OPENXR_SDK`, and pass built DLL SHA-256 == installed Steam DLL SHA-256 before a runtime launch.
- For any runtime, OpenXR, rendering, stereo, hands, weapon, input, VFX, shadow, UI, menu, performance, or deployment change, use `..\SingularityVR-production\tools\singularityvr-test-bridge.ps1` and its mandatory preflight/collection flow.
- Read `docs/VR_STATUS.md` and `docs/VR_FEATURE_GAP_ANALYSIS.md` before choosing work. In the production worktree, read those files there as the current evidence source.
- Do not guess engine offsets or pass identity. Use one runtime variable at a time, bounded diagnostics, and no per-draw allocation/logging.
- Track evidence as L1/L2/L3/L4. A compile, counter, simulator pass, or initialization line alone does not prove physical behavior. Do not call a feature fixed without its required evidence; do not make the user the first-line debugger.
- Reserve headset acceptance for perceptual questions after autonomous tests pass. No `PHYSICALLY_VERIFIED` claim without a physical HMD result.
- Project rules override generic skills. Skills are under `.agents/skills/`; read the test-bridge skill for runtime work.
