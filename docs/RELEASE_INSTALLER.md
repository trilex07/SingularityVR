# SingularityVR release installer (Side deliverable)

Offline installer + verifier + localization backlog. No Steam writes outside
`-GamePath`; all validation runs against synthetic trees by default.

## Quick start (synthetic self-test)

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/release/tests/Test-Installer.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools/release/tests/Test-Verifier.ps1
py -3 tools/localization/analyze_gaps.py --out tools/localization
```

Expect: installer 20/20, verifier 4/4, analyzer `records=2637`.

## Install into a real game (MAIN-owned operation)

```powershell
.\tools\release\Install-SingularityVR.ps1 -GamePath 'C:\...\Singularity' `
  -Profile Core,Localization -AssetRoot <staging> [-AllowExperimental] [-ForceUnknownGameBuild]
.\tools\release\Install-SingularityVR.ps1 -GamePath <dir> -Mode Repair
.\tools\release\Install-SingularityVR.ps1 -GamePath <dir> -Mode Uninstall
.\tools\release\Verify-ReleaseAssets.ps1 -AssetRoot <staging>
```

Behavior: game-dir + `Singularity.exe` check, pinned-build gate (fails
closed unless the manifest pins the hash or `-ForceUnknownGameBuild` is
explicit), source availability + x86 PE32 check before touching anything,
timestamped backup of every overwritten file, INI merge (user keys win,
missing defaults appended), `SingularityVR-installed.json` ownership
manifest, verified rollback on any failure, uninstall restores originals
and removes only installer-owned files.

## For MAIN: adding a component

1. Append an entry to `tools/release/release_manifest.json` (fields per
   existing entries; `dest` must stay inside the game dir; tree kinds ship
   as `AssetRoot/<id>/`).
2. Single-file build/repo/sdk kinds resolve automatically; generated trees
   need staging under `-AssetRoot`.
3. Extend `Test-Installer.ps1`/`Test-Verifier.ps1` with a case and re-run.
4. Pin `Singularity.exe` SHA-256 into `manifest.game.builds` once a
   supported build is confirmed (until then installs need the explicit flag).
5. Experimental assets stay behind `-AllowExperimental` until accepted;
   never flip defaults in this branch.

## Statuses (honest by construction)

`backlog.json`/`GAPS.md` (2637 unique + 806 repeat references): machine
drafts are NEEDS_CORRECTION, recordings are never accepted on existence/ASR,
`verified_in_game` is false everywhere (Side cannot verify). Texture 8K tier
is listed but has no content; BIK candidates live on side branches until
runtime-tested. See `tools/release/release_manifest.json` readiness fields
and `tools/localization/GAPS.md` for the actionable backlog.
