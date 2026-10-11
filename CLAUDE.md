# Singularity VR mod — working notes for Claude

`AGENTS.md` is the authoritative workflow. MAIN owns Steam deployment/integration testing; side agents work in isolated worktrees and never launch or deploy the Steam game (L1/L2 only). Headset testing below is physical PSVR2 acceptance only, separate from automatic tests — never mark headset-verified without an actual headset result.

A VR mod for Singularity (2010, Unreal Engine 3, D3D9, x86), shipping as one `d3d9.dll` dropped
beside the game exe. No game files are modified.

## Read in this order

| File | What it is | When |
|---|---|---|
| `STATUS.md` | **Current state + what to do next.** ~400 lines | Always, first |
| `ENGINE_NOTES.md` | Engine internals, addresses, structures, Ghidra findings | When touching the engine |
| `ENVIRONMENT.md` | Paths, toolchain, game copies | When a path is unclear |
| `HISTORY.md` | The run log, newest first. ~3,700 lines | **Search, don't read.** See below |

**Do not read `HISTORY.md` cold** — it is roughly 55k tokens and almost none of it is current. Go
there for one job: before building on a premise you inherited, check what actually measured it.
Sections carrying a ⛔ **SUPERSEDED** banner are wrong-but-kept; never quote them as current.

## The build (safe production workflow)

```powershell
.\spikes\view_matrix\build.ps1
```

`spikes/view_matrix/` is the live build (a single ~14,500-line `d3d9.cpp`). `spikes/vr_render/`
is the known-good fallback. Earlier spikes are kept on purpose — several record approaches that
did not work.

Do not use `build.ps1 -Install`: it copies into a game directory without the
production backup/verify path. The safe flow is build, then verify the
artifact (x86 PE32, `/O2` provenance, record branch/commit and SHA-256),
and only MAIN deploys to Steam through `tools/deploy-singularityvr.ps1`,
which creates a timestamped backup and verifies SHA-256 after each copy.
If the deployed result is wrong, MAIN rolls back with
`tools/deploy-singularityvr.ps1 -Action Rollback -BackupPath <timestamp>`.
Side agents build and verify only; they never deploy.

## The log

`%LOCALAPPDATA%\SingularityVR\`. **Read logs from disk with the Read tool.**

- `view_matrix.log` is **always the current run**. At the next attach it is archived as
  `view_matrix_YYYY-MM-DD_HH-MM-SS.log`, stamped with when it *started*.
- **Default to reading `view_matrix.log` alone.** For an A/B, name the two timestamped files
  explicitly. Never bulk-read the directory — one run can be 1.2 MB.
- Nothing is ever deleted, so a log named in a note stays valid forever. Cite them by name.
- `view_matrix.prevN.log` are from the old positional scheme. Historical; don't add to them.
- Never paste log contents into `STATUS.md` or `HISTORY.md`. The docs stay prose; the logs stay
  on disk. (`STATUS.md` reached 240 KB without this happening — don't start.)

## Testing happens in a VR headset (physical acceptance only, separate from automatic tests)

Side agents do not run headset or Steam tests. When MAIN needs a physical PSVR2 check, the user is wearing a headset and cannot read the screen during that test.

1. **Give numbered keypresses first.** Reasoning comes after, or not at all.
2. **State what would fool the test**, next to the steps — name the false-pass mode explicitly.
   A test that cannot fail proves nothing, and this project has lost multiple runs to instruments
   that were quietly lying.
3. One question per run where possible. A headset run is expensive; batching two uncontrolled
   changes into one wastes it.

## What this project keeps getting wrong

These are earned, not generic. The long-form versions are under *Method notes* in `STATUS.md`.

- **Almost every hard problem here was a stale premise, not a hard problem.** Re-test the premise
  before building on it, especially one you inherited from an earlier run.
- **Validate the instrument before trusting it.** SSW silently halved every frame-rate number for
  thirty runs. Three diagnostics were later found incapable of reporting anything at all.
- **"our work" in the frame budget is a residual, not a measurement.** It absorbs anyone's
  latency while looking exactly like your own bug.
- **The cheap control first.** Rename `d3d9.dll` to `d3d9.dll.off` and run unmodded at the same
  resolution — one minute, and it settles "is this us at all" before any bisection.

## Doc hygiene

`STATUS.md` is current state. When something ships, **update the existing claim rather than
appending a new one** — every ⛔ **SUPERSEDED** banner in `HISTORY.md` exists because a status
section was appended past instead of edited, and then read as fact months later. Run narrative
belongs in `HISTORY.md`, newest at the top.

Numbers in prose go stale within hours. Prefer "newest first" to "runs 1–179", and a rough size to
an exact line count — this file claimed both within a day of being written.

**No machine-specific absolute paths in committed files.** Write `C:\Users\<you>\...` or
`%LOCALAPPDATA%\...` — never a real home directory. `R:\` paths are fine; a username is not. The
whole history had to be rewritten once to strip two such lines out of `ENVIRONMENT.md`, and the
only reason that was cheap is that they were the only two in the repo.
