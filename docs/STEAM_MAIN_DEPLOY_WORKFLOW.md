# Steam deployment workflow (MAIN)

Only MAIN deploys to the installed Steam game through `tools/deploy-singularityvr.ps1`.
Side agents build and verify on dedicated branches; they never copy into Steam.

## Before every deploy

1. Stop `Singularity.exe` if it is running (`-StopGame` on the deploy script).
2. Back up **both** `d3d9.dll` and `SingularityVR.ini` from
   `C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries`.
   The deploy script always backs up files it replaces; for DLL-only installs,
   copy the INI into the same timestamped folder if it will not be overwritten.
3. Build x86 PE32 `/O2` from the target commit and record `d3d9.build.json`
   (branch, commit, SHA-256).
4. Install the candidate and verify the installed DLL SHA-256 equals the build
   manifest. PE timestamp changes every link, so identity is SHA + commit, not
   file size alone.

## After a successful smoke test

**Do not automatically restore the previous Steam DLL/INI.**

Leave the latest working candidate installed until physical headset verification
finishes or the user explicitly requests rollback.

If a candidate fails critically (crash, no OpenXR session, broken input), restore
from the timestamped backup with:

```powershell
tools/deploy-singularityvr.ps1 -Action Rollback -BackupPath <timestamp> -RepoRoot <worktree>
```

## Reporting

Every deploy or test report must state what Steam is running **now**:

- branch and commit
- DLL SHA-256
- whether INI was preserved or replaced
- backup path

Never mark a feature ready for PSVR2 visual verification while Steam still has
an older DLL.

## Hands-only candidate profile (installed INI, not replaced unless requested)

For `feature/hands-only-gloves` headset checks, the installed INI should keep:

- `ShowControllerArms=0`
- `GloveIsolate=2`
- `IndependentHandsPrototype=1`
- `ArmIKMode=0`
- `HudStereo=1` (via ini; mod raises hook level as needed)
