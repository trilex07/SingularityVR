[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$BackupPath,
    [Parameter(Mandatory)][string]$CandidateDllSha256,
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
