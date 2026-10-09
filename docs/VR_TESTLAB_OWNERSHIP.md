# SingularityVR Test Lab ownership

The test levels are deliberately split by authority:

| Level | Who may run it | Scope |
|---|---|---|
| L1 | sub-agent or main agent | unit, synthetic oracle, `.vrrec` dump and replay dry-run |
| L2 | sub-agent or main agent | build provenance, x86 PE32, `/O2`, SHA-256, handoff |
| L3 | main agent only | main Steam install, deliberate deployment, dvr-xrsim, `.vrrec`, fresh L/R capture and oracle |
| L4 | user | physical PSVR2 and perceptual acceptance |

## Sub-agent handoff

```powershell
pwsh -NoProfile -File .\tools\agent-l1-l2-handoff.ps1 `
  -Action L1 `
  -Recording <session.vrrec> `
  -XrSimDirectory <isolated-xrsim-dir> `
  -EvidenceDirectory <handoff-dir>

pwsh -NoProfile -File .\tools\agent-l1-l2-handoff.ps1 `
  -Action L2 `
  -Artifact <built-d3d9.dll> `
  -BuildManifest <d3d9.build.json> `
  -EvidenceDirectory <handoff-dir>

pwsh -NoProfile -File .\tools\agent-l1-l2-handoff.ps1 `
  -Action Package -EvidenceDirectory <handoff-dir> -HandoffPath <handoff.json>
```

The handoff contains the commit, L1/L2 reports, artifact SHA and the explicit
`main-agent` L3 owner. It never launches the game and never writes Steam.

## Main-agent L3

The main agent reviews the handoff and then performs the only deployment step
consciously. Before it, verify the artifact and back up the main installation:

```powershell
pwsh -NoProfile -File .\tools\l3-steam-guard.ps1 `
  -Action Verify -Artifact <built-d3d9.dll> -ExpectedSha256 <handoff-sha>

pwsh -NoProfile -File .\tools\l3-steam-guard.ps1 `
  -Action Backup -SteamRoot <steam-root> -BackupDirectory <new-backup-dir>
```

Then the main agent uses the existing project deployment procedure, launches
the main Steam installation with `dvr-xrsim`, waits for the exact game PID to
be `FOCUSED` with two projection views, replays `.vrrec`, and runs the existing
Test Bridge `Collect` and `VerifyCapture` actions. No handoff tool performs
this step.

After the bounded run, or on failure, stop only the exact game PID and restore
the recorded files explicitly:

```powershell
pwsh -NoProfile -File .\tools\l3-steam-guard.ps1 `
  -Action Rollback -SteamRoot <steam-root> `
  -BackupDirectory <backup-dir> -ConfirmRollback
```

`l3-steam-guard.ps1` intentionally has no `Deploy` or `Launch` action. Its
rollback removes only known files that were absent from the backup manifest;
it does not delete arbitrary directories.
