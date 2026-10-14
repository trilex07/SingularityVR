[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$BackupPath,
    [Parameter(Mandatory)][string]$CandidateDllSha256,
    [switch]$DllOnly,
    [switch]$ShadowReceiverExperiment,
    [switch]$ShadowClearGateExperiment,
    [switch]$ShadowWorldReceiverExperiment,
    [switch]$ShadowFirstPersonReceiverExperiment,
    [switch]$GameplayHotPathRequest,
    [switch]$InputEnumerationExperiment,
    [ValidateRange(60,1800)][int]$LaunchWindowSeconds = 600
)
# MAIN-owned single test lease: no launch/input/kill, never replace a running
# game, and never overwrite files changed by another deployment. Run hidden.
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
# Use the selected host's modules, not inherited PSModulePath from another
# PowerShell edition. The first Windows PowerShell launch lacked Get-FileHash.
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Utility') -ErrorAction Stop
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Management') -ErrorAction Stop
$backup = (Resolve-Path -LiteralPath $BackupPath).Path
$manifest = Get-Content -LiteralPath (Join-Path $backup 'manifest.json') -Raw | ConvertFrom-Json
if ($DllOnly) {
    # DLL-only placement tests retain the physically accepted native UI package.
    # Use the existing deployment runner's exact backup format and rollback.
    $savedDll=Join-Path $backup 'd3d9.dll'
    if ((Get-FileHash -LiteralPath $savedDll).Hash -ne $manifest.Sha256) { throw 'Invalid DLL restore backup' }
    $dllTarget='C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\d3d9.dll'
    $deadline=(Get-Date).AddSeconds($LaunchWindowSeconds); $seen=$false
    Write-Output "READY: verified DLL-only restore backup; launch window $LaunchWindowSeconds seconds"
    while ($true) {
        $running=@(Get-Process -Name Singularity -ErrorAction SilentlyContinue)
        if ($running.Count) { $seen=$true }
        elseif ($seen -or (Get-Date) -ge $deadline) { break }
        Start-Sleep -Seconds 2
    }
    if ($ShadowReceiverExperiment) { & (Join-Path $PSScriptRoot 'set-shadow-receiver-experiment.ps1') -State OFF }
    if ($ShadowClearGateExperiment) { & (Join-Path $PSScriptRoot 'set-shadow-receiver-experiment.ps1') -State OFF -Experiment ClearGate }
    if ($ShadowWorldReceiverExperiment) { & (Join-Path $PSScriptRoot 'set-shadow-world-experiment.ps1') -State OFF }
    if ($ShadowFirstPersonReceiverExperiment) { & (Join-Path $PSScriptRoot 'set-shadow-world-experiment.ps1') -State OFF -Experiment FirstPerson }
    if ($InputEnumerationExperiment) { & (Join-Path $PSScriptRoot 'set-input-enum-experiment.ps1') -State OFF }
    if ($GameplayHotPathRequest) {
        $ownedRequest='C:\Users\Trilex\AppData\Local\SingularityVR\hotpath-gameplay.request'
        if (Test-Path -LiteralPath $ownedRequest) {
            if ([IO.File]::ReadAllText($ownedRequest) -in @('SingularityVR gameplay hotpath v1','SingularityVR cinema exit hotpath v1')) {
                Remove-Item -LiteralPath $ownedRequest
                Write-Output 'Removed unconsumed owned gameplay hotpath request'
            } else { Write-Warning 'Unknown gameplay hotpath request preserved' }
        }
    }
    if ((Get-FileHash -LiteralPath $dllTarget).Hash -ne $CandidateDllSha256) { throw 'Installation changed; refusing DLL restore' }
    & (Join-Path $PSScriptRoot 'deploy-singularityvr.ps1') -Action Rollback -BackupPath $backup
    if ((Get-FileHash -LiteralPath $dllTarget).Hash -ne $manifest.Sha256) { throw 'DLL rollback verification failed' }
    Write-Output 'RESTORED: accepted DLL hash verified; native UI package untouched'
    exit 0
}
$expected = @{}
foreach ($entry in $manifest.files) {
    $saved = Join-Path $backup $entry.name
    if ((Get-FileHash -LiteralPath $saved -Algorithm SHA256).Hash -ne $entry.sha256) {
        throw "Invalid restore backup: $saved"
    }
    $expected[$entry.sourcePath] = $entry.sha256
    if ($entry.name -eq 'd3d9.dll') { $expected[$entry.sourcePath] = $CandidateDllSha256 }
    if ($entry.name -eq 'UI_SinglePlayer_SF.xxx') {
        $expected[$entry.sourcePath] = '5B2EA469A3EE06FF154AC78BD58604E8034970D4837F289E5C22503C6A673CDE'
    }
}
$deadline = (Get-Date).AddSeconds($LaunchWindowSeconds)
Write-Output "READY: verified restore backup; launch window $LaunchWindowSeconds seconds"
$seen = $false
while ($true) {
    $running = @(Get-Process -Name Singularity -ErrorAction SilentlyContinue)
    if ($running.Count) { $seen = $true }
    elseif ($seen -or (Get-Date) -ge $deadline) { break }
    Start-Sleep -Seconds 2
}
foreach ($target in $expected.Keys) {
    if ((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -ne $expected[$target]) {
        throw "Installation changed since candidate lease; refusing restore: $target"
    }
}
# Runner rechecks processes, file locks, staged copies and restored hashes.
& (Join-Path $PSScriptRoot 'test-pause-rtt-package.ps1') -Action Restore -BackupPath $backup
foreach ($entry in $manifest.files) {
    if ((Get-FileHash -LiteralPath $entry.sourcePath -Algorithm SHA256).Hash -ne $entry.sha256) {
        throw "Post-restore verification failed: $($entry.sourcePath)"
    }
}
Write-Output 'RESTORED: all original file hashes verified'
