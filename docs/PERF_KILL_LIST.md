# Performance kill list — manual checklist + A/B procedure

Code defaults are already safe (`TimeOurWork=0`, `HotPathSampler=0`,
`[Diagnostic] Enabled=0`, readout hidden, `ZeroCopy=1`, `FrameCopyMode=1`).
The remaining items live outside the mod: engine settings, runtime settings,
and measurement discipline. Manual steps first — do not fragile-patch what a
checkbox fixes (vrframework guide 14, section 5).

## Manual kill list (do before every A/B)

- [ ] Headset at 120 Hz (90 Hz only if the PC cannot hold 120 at lower render scale).
- [ ] Render scale: lower first, resolution second. Quest/VDXR is the tested path.
- [ ] In-game: vsync OFF, frame cap OFF, frame generation OFF, dynamic
      resolution OFF, motion blur OFF, depth of field OFF.
- [ ] Desktop mirror / recorder overlays OFF during measurement runs.
- [ ] UE3 smoother: if frames lock at a round stable number with unchanged draw
      count, suspect `bSmoothFrameRate` governing — NUMPAD+ neutralises the
      render-thread sleep for one-run A/B (log says which).
- [ ] One variable per run. Changing ini + headset + scene together proves nothing.

## A/B procedure with the sampler (tools in SingularityVR-tools/code-tools)

1. Baseline run: `HotPathSampler=1` in `[Render]`, play the SAME scene ~2 min,
   quit through the menu (clean save), copy
   `%LOCALAPPDATA%\SingularityVR\view_matrix_<stamp>.log` aside (never compare
   against a truncated live log).
2. One change. Candidate run, same scene, same duration.
3. `python perf_compare.py baseline.log candidate.log --markdown cmp.md`
   Read PERFORMANCE REGRESSION RANKING (delta ms/frame per family).
4. Families present in only one log are shown, never ranked. A missing sampler
   window (mod rebuilt without it, run too short) invalidates the comparison.

## Notes

- FSR-style upscaling is NOT in this build. It is a renderer feature, not a
  lever — listed here only so nobody waits for it. The cheap equivalents that
  exist today: lower render scale, `ZeroCopy=1`, mirror off.
- `present-hook residual estimate` in the sampler report is the unaccounted
  remainder: if it dominates, the cost is outside all instrumented families —
  look at the game thread (smoother sleep, input processing), not the hooks.
- SLOW-frame wall/CPU rows separate GPU-bound (wall grows, CPU flat) from
  CPU-bound (both grow). Fix the right one.
