[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$BackupPath,
    [Parameter(Mandatory)][string]$AcceptedProfilePath,
    [Parameter(Mandatory)][string]$ChangePlanPath,
    [Parameter(Mandatory)][ValidatePattern('^[0-9A-Fa-f]{64}$')][string]$AcceptedIniSha256,
    [ValidateRange(60, 1800)][int]$LaunchWindowSeconds = 600,
    [switch]$PreflightOnly,
    [string]$SteamRoot = 'C:\Program Files (x86)\Steam'
)

# MAIN test lease: read-only until the game has exited; never launches or kills it.
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$backup = (Resolve-Path -LiteralPath $BackupPath).Path
$profilePath = (Resolve-Path -LiteralPath $AcceptedProfilePath).Path
$planPath = (Resolve-Path -LiteralPath $ChangePlanPath).Path
$profile = Get-Content -LiteralPath $profilePath -Raw | ConvertFrom-Json
$plan = Get-Content -LiteralPath $planPath -Raw | ConvertFrom-Json
$metadata = Get-Content -LiteralPath (Join-Path $backup 'backup_metadata.json') -Raw | ConvertFrom-Json
if ($profile.schema -cne 'singularityvr-accepted-profile-v1' -or $profile.approved -ne $true -or
    $plan.schema -cne 'singularityvr-change-plan-v1' -or
    $plan.profileSha256 -cne (Get-FileHash -LiteralPath $profilePath -Algorithm SHA256).Hash) {
    throw 'Wrong accepted profile or change plan'
}
$names = @($metadata.Files | ForEach-Object Name | Sort-Object)
if ($names.Count -ne 3 -or ($names -join ',') -cne 'd3d9.dll,dinput8.dll,SingularityVR.ini' -or
    @($metadata.Directories).Count -ne 0) { throw 'Restore scope must be exactly DLL, injector and INI' }
$gameRoot = Join-Path $SteamRoot 'steamapps\common\Singularity'
$gameBin = Join-Path $gameRoot 'Binaries'
$accepted = @{}
foreach ($name in $names) {
    $relative = 'Binaries/' + $name
    $entry = @($profile.files | Where-Object { $_.path -ceq $relative })
    $metadataEntry = @($metadata.Files | Where-Object { $_.Name -ceq $name })
    $saved = Join-Path $backup $name
    $expectedSha = if ($name -eq 'SingularityVR.ini') { $AcceptedIniSha256.ToUpperInvariant() }
                   elseif ($entry.Count -eq 1) { $entry[0].sha256 }
                   else { throw "Missing accepted profile file: $name" }
    if ($metadataEntry.Count -ne 1 -or $metadataEntry[0].Sha256 -cne $expectedSha -or
        (Get-FileHash -LiteralPath $saved -Algorithm SHA256).Hash -cne $expectedSha) {
        throw "Accepted backup mismatch: $name"
    }
    $accepted[$name] = $expectedSha
}
$dllChange = @($plan.files | Where-Object { $_.path -ceq 'Binaries/d3d9.dll' })
if ($dllChange.Count -ne 1 -or $dllChange[0].before -cne $accepted['d3d9.dll']) {
    throw 'Plan DLL does not start at accepted hash'
}
$candidate = @{
    'd3d9.dll' = $dllChange[0].after
    'dinput8.dll' = $accepted['dinput8.dll']
    'SingularityVR.ini' = (Get-FileHash -LiteralPath (Join-Path (Split-Path $planPath) 'SingularityVR.ini') -Algorithm SHA256).Hash
}
$ui = Join-Path $gameRoot 'RvGame\CookedPC\UI_SinglePlayer_SF.xxx'
$uiSha = (Get-FileHash -LiteralPath $ui -Algorithm SHA256).Hash
foreach ($name in $names) {
    if ((Get-FileHash -LiteralPath (Join-Path $gameBin $name) -Algorithm SHA256).Hash -cne $candidate[$name]) {
        throw "Candidate not installed or changed: $name"
    }
}
if (-not (Get-CimInstance Win32_Process -Filter "ProcessId=$PID" -ErrorAction Stop)) {
    throw 'Full process visibility required'
}
if ($PreflightOnly) {
    Write-Output 'PASS: exact backup, installed candidate, protected UI and process visibility verified; no wait or writes'
    return
}
Write-Output "READY: native-muzzle restore lease armed; launch window $LaunchWindowSeconds seconds"
$deadline = (Get-Date).AddSeconds($LaunchWindowSeconds)
$seen = $false
$idleSince = $null
while ($true) {
    $running = @(Get-CimInstance Win32_Process -Filter "Name='Singularity.exe'" -ErrorAction Stop)
    if ($running.Count) { $seen = $true; $idleSince = $null }
    elseif ($seen) {
        if (-not $idleSince) { $idleSince = Get-Date }
        if (((Get-Date) - $idleSince).TotalSeconds -ge 10) { break }
    } elseif ((Get-Date) -ge $deadline) { break }
    Start-Sleep -Seconds 2
}
if (@(Get-CimInstance Win32_Process -Filter "Name='Singularity.exe'" -ErrorAction Stop).Count) {
    throw 'Game still running; refusing restore'
}
if ((Get-FileHash -LiteralPath $ui -Algorithm SHA256).Hash -cne $uiSha) {
    throw 'Protected UI changed during test'
}
foreach ($name in $names) {
    if ((Get-FileHash -LiteralPath (Join-Path $gameBin $name) -Algorithm SHA256).Hash -cne $candidate[$name]) {
        throw "Installation changed during test; refusing restore: $name"
    }
}
& (Join-Path $repoRoot 'tools\deploy-singularityvr.ps1') -Action Rollback -BackupPath $backup `
    -RepoRoot $repoRoot -AcceptedProfilePath $profilePath -ChangePlanPath $planPath
foreach ($name in $names) {
    if ((Get-FileHash -LiteralPath (Join-Path $gameBin $name) -Algorithm SHA256).Hash -cne $accepted[$name]) {
        throw "Accepted restore mismatch: $name"
    }
}
if ((Get-FileHash -LiteralPath $ui -Algorithm SHA256).Hash -cne $uiSha) { throw 'Protected UI changed during restore' }
Write-Output 'RESTORED: accepted DLL, injector and INI hashes verified; UI untouched'
