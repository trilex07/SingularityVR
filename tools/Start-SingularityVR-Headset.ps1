[CmdletBinding()]
param(
    [string]$SteamRoot = 'C:\Program Files (x86)\Steam',
    [int]$SteamVrWaitSeconds = 45,
    [int]$GameWaitSeconds = 30,
    [switch]$StopGame
)

# User launcher for the installed developer build. It starts SteamVR first,
# then launches Singularity through Steam without changing DLLs, INI files,
# OpenXR registry settings, or the SteamVR configuration.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$steamExe = Join-Path $SteamRoot 'steam.exe'
$vrMonitor = Join-Path $SteamRoot 'steamapps\common\SteamVR\bin\win64\vrmonitor.exe'
if (-not (Test-Path -LiteralPath $steamExe -PathType Leaf)) {
    throw "Steam not found: $steamExe"
}
if (-not (Test-Path -LiteralPath $vrMonitor -PathType Leaf)) {
    throw "SteamVR monitor not found: $vrMonitor"
}

if ($StopGame) {
    Get-Process -Name Singularity -ErrorAction SilentlyContinue |
        Stop-Process -Force -ErrorAction SilentlyContinue
}

if (-not (Get-Process -Name steam -ErrorAction SilentlyContinue)) {
    Start-Process -FilePath $steamExe | Out-Null
    Start-Sleep -Seconds 3
}

$server = Get-Process -Name vrserver -ErrorAction SilentlyContinue
$compositor = Get-Process -Name vrcompositor -ErrorAction SilentlyContinue
if (-not $server -or -not $compositor) {
    Start-Process -FilePath $vrMonitor | Out-Null
}

$deadline = (Get-Date).AddSeconds($SteamVrWaitSeconds)
do {
    Start-Sleep -Milliseconds 500
    $server = Get-Process -Name vrserver -ErrorAction SilentlyContinue
    $compositor = Get-Process -Name vrcompositor -ErrorAction SilentlyContinue
} while ((Get-Date) -lt $deadline -and (-not $server -or -not $compositor))

if (-not $server -or -not $compositor) {
    Write-Warning 'SteamVR did not report both vrserver and vrcompositor before timeout.'
}

$steamExe = Join-Path $SteamRoot 'steam.exe'
Start-Process -FilePath $steamExe -ArgumentList @('-applaunch', '42670') | Out-Null
Write-Host 'Singularity launch requested via Steam -applaunch (not steam://).'
Write-Host 'In the game window press Enter repeatedly until gameplay appears.'
Start-Sleep -Seconds $GameWaitSeconds
