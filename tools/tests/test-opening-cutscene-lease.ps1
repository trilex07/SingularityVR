[CmdletBinding()]
param([Parameter(Mandatory)][string]$ManifestPath,
      [Parameter(Mandatory)][string]$OutputRoot,
      [Parameter(Mandatory)][string]$BaselineBackup,
      [string]$RealSteamRoot='C:\Program Files (x86)\Steam',
      [switch]$WatchTimeout)
# Offline sandbox only: fake Steam directory; never starts a game.
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$out=[IO.Path]::GetFullPath($OutputRoot)
if(Test-Path -LiteralPath $out){throw 'Test output already exists'}
$repo=(Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
if(-not $out.StartsWith($repo+'\deployment\hands-dubbing\',[StringComparison]::OrdinalIgnoreCase)){throw 'Test output must be private owned sandbox'}
$fake=Join-Path $out 'fake-steam'
$gameRoot=Join-Path $fake 'steamapps\common\Singularity'
$realGame=Join-Path $RealSteamRoot 'steamapps\common\Singularity'
$manifest=Get-Content -LiteralPath $ManifestPath -Raw | ConvertFrom-Json
foreach($entry in $manifest.Files) {
    $dst=Join-Path $gameRoot $entry.RelativePath
    New-Item -ItemType Directory -Path (Split-Path $dst -Parent) -Force | Out-Null
    $name=[IO.Path]::GetFileName($entry.RelativePath)
    $source=Join-Path $BaselineBackup $name
    if(-not (Test-Path -LiteralPath $source)){$source=Join-Path $realGame $entry.RelativePath}
    if((Get-FileHash -LiteralPath $source).Hash -cne $entry.BaselineSha256){throw 'Test baseline mismatch'}
    Copy-Item -LiteralPath $source -Destination $dst
}
$ui='RvGame\CookedPC\UI_SinglePlayer_SF.xxx'
Copy-Item -LiteralPath (Join-Path $realGame $ui) -Destination (Join-Path $gameRoot $ui)
$runner=Join-Path $repo 'tools\deploy-singularityvr.ps1'
& $runner -Action Install -OpeningCutsceneManifestPath $ManifestPath -SteamRoot $fake -WhatIf
foreach($entry in $manifest.Files){if((Get-FileHash -LiteralPath (Join-Path $gameRoot $entry.RelativePath)).Hash -cne $entry.BaselineSha256){throw 'WhatIf changed file'}}
$output=@(& $runner -Action Install -OpeningCutsceneManifestPath $ManifestPath -SteamRoot $fake)
$output | Write-Output
$backupLine=@($output | Where-Object { $_ -is [string] -and $_.StartsWith('BACKUP: ') })
if($backupLine.Count -ne 1){throw 'Missing exact backup identity'}
$backup=$backupLine[0].Substring(8)
foreach($entry in $manifest.Files){if((Get-FileHash -LiteralPath (Join-Path $gameRoot $entry.RelativePath)).Hash -cne $entry.CandidateSha256){throw 'Candidate mismatch'}}
# Corrupt a copy of the manifest: unknown target rejected before any writes.
$bad=Get-Content -LiteralPath $ManifestPath -Raw | ConvertFrom-Json
$bad.Files[0].RelativePath='..\escape.dll'
$badPath=Join-Path $out 'bad-manifest.json'
$bad | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $badPath -Encoding UTF8
$refused=$false
try { & $runner -Action Install -OpeningCutsceneManifestPath $badPath -SteamRoot $fake } catch { $refused=$true }
if(-not $refused){throw 'Path escape accepted'}
& $runner -Action Rollback -OpeningCutsceneManifestPath $ManifestPath -SteamRoot $fake -BackupPath $backup
foreach($entry in $manifest.Files){if((Get-FileHash -LiteralPath (Join-Path $gameRoot $entry.RelativePath)).Hash -cne $entry.BaselineSha256){throw 'Baseline restore mismatch'}}
if((Get-FileHash -LiteralPath (Join-Path $gameRoot $ui)).Hash -cne $manifest.FrozenUiSha256){throw 'UI changed'}
# Exercise backup-corruption refusal and preserve an unexpected user INI write.
$output=@(& $runner -Action Install -OpeningCutsceneManifestPath $ManifestPath -SteamRoot $fake)
$backup=@($output | Where-Object { $_ -is [string] -and $_.StartsWith('BACKUP: ') })[0].Substring(8)
$badBackup=Join-Path $out 'bad-backup'
New-Item -ItemType Directory -Path $badBackup | Out-Null
Get-ChildItem -LiteralPath $backup -File | ForEach-Object {Copy-Item -LiteralPath $_.FullName -Destination $badBackup}
Set-Content -LiteralPath (Join-Path $badBackup 'SingularityVR.ini') -Value 'corrupt sandbox backup' -Encoding ASCII
$refused=$false
try { & $runner -Action Rollback -OpeningCutsceneManifestPath $ManifestPath -SteamRoot $fake -BackupPath $badBackup } catch { $refused=$true }
if(-not $refused){throw 'Corrupt backup accepted'}
foreach($entry in $manifest.Files){if((Get-FileHash -LiteralPath (Join-Path $gameRoot $entry.RelativePath)).Hash -cne $entry.CandidateSha256){throw 'Corrupt backup caused partial write'}}
$ini=Join-Path $gameRoot 'Binaries\SingularityVR.ini'
Set-Content -LiteralPath $ini -Value 'synthetic user INI change' -Encoding ASCII
$iniHash=(Get-FileHash -LiteralPath $ini).Hash
$refused=$false
try { & $runner -Action Rollback -OpeningCutsceneManifestPath $ManifestPath -SteamRoot $fake -BackupPath $backup } catch { $refused=$true }
if(-not $refused -or (Get-FileHash -LiteralPath $ini).Hash -cne $iniHash){throw 'User INI write overwritten or false restore claim'}
foreach($entry in $manifest.Files) {
    if($entry.RelativePath.EndsWith('.ini')){continue}
    if((Get-FileHash -LiteralPath (Join-Path $gameRoot $entry.RelativePath)).Hash -cne $entry.BaselineSha256){throw 'Owned candidate not restored after INI conflict'}
}
# Only the known fake installation is reconciled here, never the real game.
Copy-Item -LiteralPath (Join-Path $backup 'SingularityVR.ini') -Destination $ini -Force
Write-Output 'PASS: fake lease, WhatIf, path refusal, backup corruption, user-INI preservation, owned rollback, frozen UI'
if($WatchTimeout) {
    $output=@(& $runner -Action Install -OpeningCutsceneManifestPath $ManifestPath -SteamRoot $fake)
    $backup=@($output | Where-Object { $_ -is [string] -and $_.StartsWith('BACKUP: ') })[0].Substring(8)
    & (Join-Path $repo 'tools\watch-opening-cutscene-restore.ps1') -ManifestPath $ManifestPath `
        -BackupPath $backup -SteamRoot $fake -LaunchWindowSeconds 60
    foreach($entry in $manifest.Files){if((Get-FileHash -LiteralPath (Join-Path $gameRoot $entry.RelativePath)).Hash -cne $entry.BaselineSha256){throw 'Timeout failed to restore'}}
    Write-Output 'PASS: no-launch lease timeout restored all four files'
}
