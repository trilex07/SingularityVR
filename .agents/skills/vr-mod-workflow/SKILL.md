---
name: vr-mod-workflow
description: Project workflow for implementing and stabilizing SingularityVR features with explicit evidence gates and regression coverage.
metadata:
  source: VR-Mod-Vibecoding-Wizard
---

# SingularityVR VR mod workflow

This project is a native UE3/D3D9 x86 VR conversion. Treat `AGENTS.md`, `docs/VR_STATUS.md`, and the current source/log evidence as authoritative; older status prose can be stale.

- Before implementation, state the observed failure, hypothesis, supporting evidence, and a falsification test in the relevant issue note.
- Preserve the one-DLL proxy architecture and the known x86/OpenXR build route. Do not import engine-specific offsets from other games or frameworks.
- Implement the smallest reversible change behind a safe default where practical. Add deterministic regression coverage for meaningful logic; use the mandatory test bridge for runtime-affecting work.
- Verify the complete path: event/input → game hook → state/policy → render or backend → submitted stereo output. A valid pose or successful call is not visual proof.
- Recheck the relevant lifecycle: boot, gameplay, pause/resume, return to menu, re-enter gameplay, transitions, and shutdown as applicable.
- Keep statuses exact and evidence-linked. If a perceptual fact remains, finish all autonomous levels before handing over a short, binary headset check.

## Test ownership

- A sub-agent's deliverable is L1/L2: deterministic tests, replay dry-run,
  PE32/O2/SHA/provenance, and a handoff manifest. It never modifies the Steam
  installation or launches the main-install game.
- L3 belongs only to the main agent and uses the main Steam installation with
  explicit backup, deliberate deployment, dvr-xrsim, canonical `.vrrec`, fresh
  L/R capture, and `vr_capture_oracle.py`.
- Use `tools/l3-steam-guard.ps1` for artifact verification and backup/rollback.
  It has no deployment action. Never turn a sub-agent handoff into an implicit
  DLL install.
