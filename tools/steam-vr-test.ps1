[CmdletBinding()]
param(
    [string]$SteamRoot = 'C:\Program Files (x86)\Steam',
    [string]$LogRoot = '',
    [int]$SteamVrWaitSeconds = 45,
    [int]$GameWaitSeconds = 30,
    [switch]$StopGame
)

# Normal headset launch. This starts the installed SteamVR runtime, waits for
# vrserver and vrcompositor, then delegates to the non-invasive Steam launcher.
# It never changes OpenXR registry values, SteamVR settings, DLLs, or INI files.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$steamExe = Join-Path $SteamRoot 'steam.exe'
$steamVrBin = Join-Path $SteamRoot 'steamapps\common\SteamVR\bin\win64'
$vrMonitor = Join-Path $steamVrBin 'vrmonitor.exe'
if (-not (Test-Path -LiteralPath $vrMonitor -PathType Leaf)) {
    throw "SteamVR monitor not found: $vrMonitor"
}
if (-not (Get-Process -Name steam -ErrorAction SilentlyContinue)) {
    if (-not (Test-Path -LiteralPath $steamExe -PathType Leaf)) {
        throw "Steam not found: $steamExe"
    }
    Start-Process -FilePath $steamExe | Out-Null
    Start-Sleep -Seconds 3
}

if ($StopGame) {
    Get-Process -Name Singularity -ErrorAction SilentlyContinue | Stop-Process -Force
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
    Write-Warning 'SteamVR compositor did not report ready before timeout; the game may fall back to flat mode.'
} else {
    Write-Host ("SteamVR ready: vrserver PID {0}, vrcompositor PID {1}" -f $server.Id, $compositor.Id)
}

$launcher = Join-Path $PSScriptRoot 'steam-ui-smoke-test.ps1'
& $launcher -SteamRoot $SteamRoot -LogRoot $LogRoot -WaitSeconds $GameWaitSeconds -StopGame:$StopGame
