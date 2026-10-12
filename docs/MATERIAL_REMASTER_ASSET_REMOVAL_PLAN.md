# Material remaster — asset removal plan (PROPOSAL, not executed)

**Status:** plan only. Nothing below has been run. Execution rewrites branch
history and needs explicit MAIN approval first.
**Reason:** `material-remaster/` commits 18 game-derived binary files
(9 DDS + 9 comparison PNG, ~10.06 MiB) to a publicly distributed branch.
Licence notes mark them review-only / not licensed for redistribution, but
notes do not remove the exposure — the bytes are in the branch and its history.

## What must go

18 files under `material-remaster/`:

- 9 × `*.dds` (6 injectable `*_2x.dds` + 3 `*_specular_review.dds`)
- 9 × `comparison/*_compare.png`

## What must stay (reproducibility preserved)

- Scripts: `asset-tools/textures/dds_io.py`,
  `asset-tools/textures/material_remaster.py`,
  `asset-tools/textures/verify_true_mip0.py`
- Tests: `asset-tools/textures/tests/test_dds_io.py`,
  `test_normal_map.py`, `test_material_remaster.py`,
  `asset-tools/texture-injector/tests/test_tier_naming.py`,
  `quality_suffix.cpp`
- Injector changes: `texq_naming.h`, `texture_injector.cpp`, `build.ps1`,
  `texture_override.ini`, injector `README.md`
- Docs: `docs/MATERIAL_REMASTER.md`,
  `docs/MATERIAL_REMASTER_INTEGRATION_READINESS.md`, this plan, the handoff
- Manifests: `material-remaster/*/manifest.json`, `manifest_summary.json`,
  `material-remaster/README.md` — these contain dimensions, statistics,
  SHA-256/FNV hashes and notes only; spot-checked to contain no pixel arrays
  or base64 image data. They are the proof trail and stay.

Regeneration is deterministic (fixed seeds): re-running
`material_remaster.py` against the local sources reproduces byte-identical
DDS, verifiable against the manifest SHA-256 values.

## Recommended two-stage execution

**Stage 1 — stop the bleeding (ordinary commit, no rewrite):**

```powershell
git rm 'material-remaster/**/*.dds' 'material-remaster/**/comparison/*.png'
git commit -m "chore(material-remaster): remove game-derived binaries, keep scripts+hashes"
git push origin feature/material-remaster-v2
```

After this, fresh clones fetch no derived pixels. History still contains them
until stage 2.

**Stage 2 — purge history (MAIN approval required):**

```powershell
# fresh clone, never the working tree
pip install git-filter-repo
git filter-repo --path material-remaster/weapon_shotgun --path material-remaster/glove `
  --path material-remaster/env_door --invert-paths \
  --paths-from-file <(printf 'material-remaster/\n')  # placeholder: see note
```

Precise variant (keep manifests + README, drop binaries only) — filter-repo
has no extension filter, so list the 18 paths explicitly via
`--paths-from-file keep.txt --invert-paths` where `keep.txt` is generated
from `git ls-files 'material-remaster/*' | grep -Ev '\.(dds|png)$'`
inverted appropriately, or simpler:

```powershell
git ls-files 'material-remaster/*.dds' 'material-remaster/*/*.dds' \
  'material-remaster/*/comparison/*.png' > remove.txt
git filter-repo --paths-from-file remove.txt --invert-paths
```

Then:

1. Verify: `git log --all -- 'material-remaster/**/*.dds'` empty;
   `git ls-files 'material-remaster/*'` shows only `.json`/`.md`;
   `git show` of every kept file byte-identical to pre-rewrite
   (compare SHA-256 of `dds_io.py`, `material_remaster.py`, all tests,
   all manifests, all docs).
2. Re-run `test_dds_io.py` + `test_normal_map.py` (need no binaries — must pass).
   `test_material_remaster.py` / `test_tier_naming.py` require regenerated
   outputs; re-run `material_remaster.py` locally first, then the tests.
3. Force-push **only** `feature/material-remaster-v2` (never main, never
   another agent's branch), then re-verify the PR diff shows no binaries.

## Constraints on execution

- Do not run stage 2 while MAIN's shadow work references this branch.
- Do not touch `origin/main`, the accepted hands build, Steam, or any other
  worktree/branch.
- Coordinate with MAIN: a history rewrite invalidates existing clones of this
  branch; announce before force-pushing.
- If in doubt, stage 1 alone is sufficient to stop new distribution; stage 2
  can wait until the research is fully closed.
