[CmdletBinding()]
param(
    [string]$SteamRoot = 'C:\Program Files (x86)\Steam',
    [int]$AppId = 42670,
    [int]$WaitSeconds = 30,
    [string]$LogRoot = '',
    [switch]$StopGame
)

# Normal, non-invasive Steam launch. This script never copies DLLs, edits the
# game INI, changes SteamVR settings, or sets XR_RUNTIME_JSON. It is intended
# for the same path a user takes from the Steam Library.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$reportDir = Join-Path $repoRoot 'deployment\steam-ui-tests'
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$reportPath = Join-Path $reportDir "steam-ui-$stamp.json"
$gameExe = Join-Path $SteamRoot 'steamapps\common\Singularity\Binaries\Singularity.exe'
$logRoot = if ($LogRoot) { $LogRoot } else { Join-Path $env:LOCALAPPDATA 'SingularityVR' }

New-Item -ItemType Directory -Force -Path $reportDir | Out-Null
if (-not (Test-Path -LiteralPath $gameExe -PathType Leaf)) {
    throw "Singularity.exe not found: $gameExe"
}

if ($StopGame) {
    Get-Process -Name Singularity -ErrorAction SilentlyContinue | Stop-Process -Force
}

$steamExe = Join-Path $SteamRoot 'steam.exe'
if (-not (Get-Process -Name steam -ErrorAction SilentlyContinue)) {
    if (-not (Test-Path -LiteralPath $steamExe -PathType Leaf)) {
        throw "Steam not found: $steamExe"
    }
    Start-Process -FilePath $steamExe | Out-Null
    Start-Sleep -Seconds 3
}

$startedAt = Get-Date
Start-Process -FilePath ("steam://rungameid/{0}" -f $AppId) | Out-Null

$game = $null
$deadline = (Get-Date).AddSeconds($WaitSeconds)
while ((Get-Date) -lt $deadline) {
    $game = Get-Process -Name Singularity -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($game) { break }
    Start-Sleep -Milliseconds 500
}

$logs = @(Get-ChildItem -LiteralPath $logRoot -Filter 'view_matrix*.log' -File -ErrorAction SilentlyContinue |
    Where-Object { $_.LastWriteTime -ge $startedAt.AddSeconds(-2) } |
    Sort-Object LastWriteTime -Descending)
$report = [ordered]@{
    timestamp = $startedAt.ToString('o')
    launch = 'steam://rungameid/{0}' -f $AppId
    game_exe = $gameExe
    launched = [bool]$game
    pid = if ($game) { $game.Id } else { $null }
    log = if ($logs) { $logs[0].FullName } else { $null }
    capture_directory = $logRoot
    note = 'Use L3+R3 for one backbuffer BMP. Start/stop .vrrec separately from VR Settings.'
}
$report | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $reportPath -Encoding UTF8
$report | ConvertTo-Json -Depth 4
Write-Host "Report: $reportPath"
