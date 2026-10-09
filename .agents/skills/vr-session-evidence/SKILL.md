---
name: vr-session-evidence
description: Record, replay, and validate SingularityVR VR sessions with xrsim, .vrrec, per-eye captures, frame pacing, and bounded evidence collection before headset testing.
license: MIT
compatibility: codex, opencode
metadata:
  project: SingularityVR
  source: adapted-from-bioshock-vr-and-dishonored-vr
---

# VR session and per-eye evidence

Use for automated runtime tests, OpenXR simulation, `.vrrec` recording/replay, per-eye screenshots, frame pacing, Steam smoke tests, or evidence collection. This skill complements `singularityvr-agent-test-bridge`; it does not replace it.

## Test order

1. L1: run offline unit/synthetic tests and `tools/vr_session_dump.py` against a disposable copy of the recording.
2. L2: prove x86 PE32, `/O2`, branch/commit, build provenance, and SHA-256 before any launch.
3. L3: use the project xrsim route in an isolated data directory; assert `FOCUSED`, advancing frames, and two projection views. Then run the narrowest `.vrrec` replay and per-eye capture. Only after that run the Steam smoke route with the mandatory test bridge.
4. L4: reserve physical PSVR2 for comfort, scale, judder, controller feel, or other facts the simulator cannot establish.

## .vrrec discipline

- Treat the recording as immutable evidence. `tools/vr_session_dump.py` is read-only; write CSV/report output beside a copied input or in a disposable evidence directory.
- Validate the header, version, sample size, monotonic frames/timestamps, bounded duration, and event/transition counts before replay.
- Use `tools/xrsim_replay_vr_session.py --dry-run` first. Replay only after xrsim reports a focused/visible session; preserve atomic command batches and acknowledgement checks.
- A replay proves input/state plumbing, not visual correctness. Pair it with log assertions and per-eye captures.

## Per-eye capture contract

- Capture left and right images from the simulator/compositor, not only the desktop mirror. Include state JSON and the exact command/replay report.
- Require fresh frame/pair identifiers, non-empty images, eye separation in the expected band, and both eyes changing after a controlled head yaw.
- Compare left-vs-right for same-world stereo and before-vs-after yaw for motion. A mono pair can pass naïve image-difference checks, so retain the pair/present counters and reject stale or one-sided submits.
- Keep capture count and resolution bounded. Never add per-draw filesystem writes.

## Frame pacing and sandboxing

- Record predicted/display time, end-frame timing, pair/eye counts, held/stale image age, and frame-drop/reprojection indicators when exposed by the bridge.
- Use one variable per trial: runtime backend, stereo method, render scale, shader policy, shadow policy, or input route—not several at once.
- Each side agent gets its own worktree, xrsim data directory, logs, captures, and process ownership. Side agents must not touch Steam installs, registry runtime selection, or another agent's evidence. The primary agent owns Steam deployment.

## Closeout

Collect only the bounded manifest, current log, xrsim state/log, per-eye captures, replay report, PE/hash/provenance data, and test output. Report L1/L2/L3/L4 separately. Never write `FIXED` from a build, a session-created line, a single screenshot, or a plausible code path alone.
