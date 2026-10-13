[CmdletBinding()]
param([Parameter(Mandatory)][string]$BackupPath,
      [Parameter(Mandatory)][string]$CandidateD3dSha256,
      [Parameter(Mandatory)][string]$CandidateInjectorSha256,
      [ValidateRange(60,1800)][int]$LaunchWindowSeconds=600)
# MAIN-only two-file lease. No Steam launch/input/kill, INI or DDS mutation.
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Utility') -ErrorAction Stop
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Management') -ErrorAction Stop
$backup=(Resolve-Path -LiteralPath $BackupPath).Path
$metadata=Get-Content -LiteralPath (Join-Path $backup 'backup_metadata.json') -Raw | ConvertFrom-Json
$manifest=@(Get-Content -LiteralPath (Join-Path $backup 'manifest.json') -Raw | ConvertFrom-Json)
$names=@($metadata.Files | ForEach-Object Name | Sort-Object)
if($names.Count -ne 2 -or ($names -join ',') -ne 'd3d9.dll,dinput8.dll' -or @($metadata.Directories).Count){throw 'Restore scope must be exactly two DLLs and zero directories'}
if($manifest.Count -ne 2){throw 'Unexpected restore manifest'}
foreach($entry in $manifest) {
    if((Get-FileHash -LiteralPath (Join-Path $backup ([IO.Path]::GetFileName($entry.Path)))).Hash -ne $entry.Sha256){throw 'Bad DLL backup'}
}
$gameBin='C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries'
$expected=@{'d3d9.dll'=$CandidateD3dSha256;'dinput8.dll'=$CandidateInjectorSha256}
$deadline=(Get-Date).AddSeconds($LaunchWindowSeconds);$seen=$false;$idleSince=$null
Write-Output 'READY: two-file restore verified; original INI/UI/DDS untouched'
while($true) {
    if(@(Get-Process -Name Singularity -ErrorAction SilentlyContinue).Count){$seen=$true;$idleSince=$null}
    elseif($seen) {
        if(-not $idleSince){$idleSince=Get-Date}
        # Steam's short startup stub must not trigger restoration before the
        # actual game arrives. Also never replace files while any game runs.
        if(((Get-Date)-$idleSince).TotalSeconds -ge 10){break}
    } elseif((Get-Date) -ge $deadline){break}
    Start-Sleep -Seconds 2
}
foreach($name in $expected.Keys){if((Get-FileHash -LiteralPath (Join-Path $gameBin $name)).Hash -ne $expected[$name]){throw "Candidate changed; refusing restore: $name"}}
& (Join-Path $PSScriptRoot 'deploy-singularityvr.ps1') -Action Rollback -BackupPath $backup
if(-not $?){throw 'Restore runner failed'}
foreach($entry in $manifest) {
    $name=[IO.Path]::GetFileName($entry.Path)
    if((Get-FileHash -LiteralPath (Join-Path $gameBin $name)).Hash -ne $entry.Sha256){throw "Restore hash failed: $name"}
}
Write-Output 'RESTORED: original d3d9.dll and dinput8.dll hashes verified'
