[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$BackupPath,
    [Parameter(Mandatory)][string]$AcceptedProfilePath,
    [Parameter(Mandatory)][string]$ChangePlanPath,
    [ValidateRange(60, 1800)][int]$LaunchWindowSeconds = 600,
    [switch]$PreflightOnly,
    [string]$SteamRoot = 'C:\Program Files (x86)\Steam'
)

# Single-use MAIN test lease. Never launches/kills the game, never writes while
# Singularity is running, and refuses restore if any candidate file was changed.
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$backup = (Resolve-Path -LiteralPath $BackupPath).Path
$profile = (Resolve-Path -LiteralPath $AcceptedProfilePath).Path
$plan = (Resolve-Path -LiteralPath $ChangePlanPath).Path
$gameBin = Join-Path $SteamRoot 'steamapps\common\Singularity\Binaries'
$expectedBackup = @{
    'd3d9.dll' = 'DADE170B579A3C9D5FBDA72327BB1BCD02B7F665DECBC592D3A5A8EF2D4F51EA'
    'dinput8.dll' = '5B948DDBDE07F16AE779C274CEBA133F7F24A218ABD9FFF2D4133FCADF729D59'
    'SingularityVR.ini' = '789E02921D80C4BB1E945FFBF28D82004778B99B8A007ED6FF9C489002635117'
}
foreach ($name in $expectedBackup.Keys) {
    $saved = Join-Path $backup $name
    if (-not (Test-Path -LiteralPath $saved -PathType Leaf) -or
        (Get-FileHash -LiteralPath $saved -Algorithm SHA256).Hash -cne $expectedBackup[$name]) {
        throw "Accepted backup missing or hash mismatch: $name"
    }
}
$expectedCandidate = @{
    'd3d9.dll' = '789749E5D4DB8FC6C603DF599A38A4FB1EC00DE374E4E2FDD469B09B9C5F361B'
    'dinput8.dll' = '5B948DDBDE07F16AE779C274CEBA133F7F24A218ABD9FFF2D4133FCADF729D59'
    'SingularityVR.ini' = '8B0B0B4CF2DB78514A44B72CF3E79CC1C48C74E5632CDA34A468CA61702CCFAF'
}
$protectedUi = Join-Path (Split-Path -Parent $gameBin) 'RvGame\CookedPC\UI_SinglePlayer_SF.xxx'
$protectedUiHash = (Get-FileHash -LiteralPath $protectedUi -Algorithm SHA256).Hash
if ($protectedUiHash -notmatch '^[0-9A-Fa-f]{64}$') { throw 'Could not pin protected UI package hash' }
foreach ($name in $expectedCandidate.Keys) {
    $live = Join-Path $gameBin $name
    if ((Get-FileHash -LiteralPath $live -Algorithm SHA256).Hash -cne $expectedCandidate[$name]) {
        throw "Candidate changed before lease was armed: $name"
    }
}
if (-not (Get-CimInstance Win32_Process -Filter "ProcessId=$PID" -ErrorAction Stop)) {
    throw 'Full process visibility required for automatic restore lease'
}
if ($PreflightOnly) {
    Write-Output 'PASS: backup, candidate, protected UI hash and full process visibility verified; no wait or writes.'
    return
}

$deadline = (Get-Date).AddSeconds($LaunchWindowSeconds)
$seen = $false
Write-Output "READY: startup/Centurion lease armed for ${LaunchWindowSeconds}s; no launch or kill."
Write-Output "UI package is not a deployment target; observed SHA-256 $protectedUiHash"
while ($true) {
    $running = @(Get-CimInstance Win32_Process -Filter "Name='Singularity.exe'" -ErrorAction Stop)
    if ($running.Count) {
        $seen = $true
    } elseif ($seen -or (Get-Date) -ge $deadline) {
        break
    }
    Start-Sleep -Seconds 2
}
if (@(Get-CimInstance Win32_Process -Filter "Name='Singularity.exe'" -ErrorAction Stop).Count) {
    throw 'Game still running; refusing to restore files in use'
}
if ((Get-FileHash -LiteralPath $protectedUi -Algorithm SHA256).Hash -cne $protectedUiHash) {
    throw 'Protected UI package changed during test; refusing automatic rollback'
}
foreach ($name in $expectedCandidate.Keys) {
    $live = Join-Path $gameBin $name
    if ((Get-FileHash -LiteralPath $live -Algorithm SHA256).Hash -cne $expectedCandidate[$name]) {
        throw "Installed file changed during test; refusing automatic rollback: $name"
    }
}

& (Join-Path $repoRoot 'tools\deploy-singularityvr.ps1') -Action Rollback `
    -BackupPath $backup -AcceptedProfilePath $profile -ChangePlanPath $plan
foreach ($name in $expectedBackup.Keys) {
    $live = Join-Path $gameBin $name
    if ((Get-FileHash -LiteralPath $live -Algorithm SHA256).Hash -cne $expectedBackup[$name]) {
        throw "Accepted restore hash mismatch: $name"
    }
}
if ((Get-FileHash -LiteralPath $protectedUi -Algorithm SHA256).Hash -cne $protectedUiHash) {
    throw 'Protected UI package changed during restore'
}
Write-Output 'RESTORED: accepted DLL, INI and dinput8 hashes verified; UI package untouched.'
