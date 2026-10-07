# SingularityVR code-tools

Reusable developer diagnostics for SingularityVR builds. Light dependencies only:
Windows PowerShell (5.1+) and Python 3 (standard library, no pip packages).

## Explicit guarantee

**NONE OF THESE TOOLS DEPLOY OR MODIFY THE GAME.**

- No tool writes to the game directory, the Steam library, or any DLL.
- No tool edits production sources (`spikes/view_matrix/d3d9.cpp`, headers, ini).
- No tool builds or launches the game.
- `build_provenance.ps1` only *reads* DLL bytes and git metadata.
- `hotpath_audit.py` only *reads* sources and prints/optionally writes a report.
- `perf_compare.py` only *reads* log files and prints/optionally writes a report.
- `gfx_recon.py` only *reads* the game binary/sources/research notes.

All tools are safe to run while another agent is working the production pass.

## Tools

### A — Build provenance verifier (`build_provenance.ps1`)

Prevents deploying a DLL built from the wrong repo/worktree/branch.

Reports: repository/worktree path, branch, HEAD SHA, `git status` (clean/dirty),
remote URL, recent commits; for the build DLL and (optionally) the deployed DLL:
path, timestamp, size, SHA-256, PE architecture (PE32/PE32+, x86/x64, subsystem).
Then prints `BUILD == DEPLOYED: YES/NO/N/A`.

It NEVER deploys anything. Exit code: 0 = match, 1 = mismatch, 2 = missing input.

Example invocations:

```powershell
# Defaults: auto-detect build DLL, compare against the Steam install
.\code-tools\build_provenance.ps1

# Explicit paths (preferred for A/B comparisons)
.\code-tools\build_provenance.ps1 -BuildDll C:\build\d3d9.dll
.\code-tools\build_provenance.ps1 -BuildDll C:\build\d3d9.dll `
    -DeployedDll "C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\d3d9.dll" `
    -Repo C:\Users\Trilex\vr-mods\SingularityVR-production
```

Expected output: sectioned report (REPOSITORY / BUILD DLL / DEPLOYED DLL /
COMPARISON) ending in `BUILD == DEPLOYED: YES|NO`.

Limitations:

- Architecture detection parses the PE headers directly; packed/corrupt files
  report `INVALID` instead of failing.
- Auto-detection only covers common output paths; pass `-BuildDll` explicitly.
- A dirty `git status` is reported, not blocked — the human decides.
- Read-only: YES.

### B — Static hot-path auditor (`hotpath_audit.py`)

Statically scans SingularityVR sources for operations that are potentially
expensive per draw call or per frame (COM getters, refcount churn, shader
reflection, file I/O / `Log()`, formatting, heap allocation, locks, QPC pairs,
`VirtualQuery`/object-graph walks, OpenXR calls in draw hooks, sleeps, ...).
Every finding carries file, line, containing function, category, likely
frequency, severity (LOW/MEDIUM/HIGH), and the rationale.

It does NOT mark every COM call a bug, and findings are NOT proven performance
bugs — they are leads a human verifies against the call path.

```cmd
python code-tools\hotpath_audit.py
python code-tools\hotpath_audit.py --root spikes\view_matrix --markdown research\code-tools\hotpath-audit.md
python code-tools\hotpath_audit.py --min-severity HIGH
```

Notes:

- Actual hook names in this tree are `Hook_DrawPrim`, `Hook_DrawIndexedPrim`,
  `Hook_DrawPrimUP`, `Hook_DrawIndexedPrimUP` (spec aliases also matched),
  `StereoPair`/`HudStereoPair`, `Hook_Present`, `UpdateFromHeadset`, etc.
- Console summary: counts by severity/category plus per-category rationale.
- `--markdown` writes the `research\code-tools\hotpath-audit.md` table.
- Read-only: YES (source is never edited).

### C — Performance session comparator (`perf_compare.py`)

Compares two session logs: `python perf_compare.py baseline.log candidate.log`.
Tolerant parser — understands `perf:` lines, `frame budget:` breakdowns,
pad-state rows, SLOW-frame CPU census rows, XR-copy/Present spans, the
`hotpath sampler complete` family table (`fam[calls=.. samples=.. avg_us=..
est_ms/frame=..]`), xinput rates, plus generic `avg_ms=` rows. Absent fields
print as `N/A` and never fail the run.

Statistics per metric: average, median, p95, p99, min/max, plus absolute and
% deltas. Hook families show calls/frame, ms/frame, avg µs/call for both
sessions. Ends with a `PERFORMANCE REGRESSION RANKING` by delta ms/frame.
Nothing is invented: families present in only one log are shown, not ranked.

```cmd
python code-tools\perf_compare.py baseline.log candidate.log
python code-tools\perf_compare.py baseline.log candidate.log --markdown report.md --csv report.csv
```

- Supported metrics: frame ms/fps, draws/frame peak, split/mono/offscreen/
  user-ptr draws, xrWaitFrame/copy/GObjects-walk/XR-submit/reg-scan/gameplay-
  hooks/our-work budget, residual, SLOW wall/CPU, XR-copy span, D3D9 Present,
  sampler families (draw/menu/foreground/shadow/ik/palette/set_texture/
  other_set/wait_frame/pose_input/d3d_copy/xr_images/xr_submit/present_frame),
  xinput rates.
- Read-only: YES.

### D — GFx / Scaleform recon (`gfx_recon.py` → `research\code-tools\gfx-recon.md`)

Bounded, read-only reconnaissance for future semantic-menu reverse engineering.
Scans `Singularity.exe`, sources, and existing research notes for Scaleform/GFx
evidence (GFxMovie, MovieImpl, SetViewport, MovieView, Advance, Display,
HandleEvent, AS2/AS3, menu names, viewport strings). Never hooks, patches, or
launches the game. Findings are separated into PROVEN / LIKELY / SPECULATIVE,
and no address is claimed as a hook target without evidence.

```cmd
python code-tools\gfx_recon.py
python code-tools\gfx_recon.py --exe "C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\Singularity.exe" --markdown research\code-tools\gfx-recon.md
```

## Test layout

- `code-tools\tests\` holds synthetic fixtures (not game data):
  `synth_baseline.log` / `synth_candidate.log` for the comparator, including
  missing-field cases.

## Git

Work happens on branch `research/code-tools`, pushed to
`origin/research/code-tools`. Production is never merged or cherry-picked.
