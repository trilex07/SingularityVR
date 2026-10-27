# After xrsim/Steam test runs, Steam may still inherit XR_RUNTIME_JSON from the test shell.
# That forces Singularity onto dvr-xrsim (flat desktop sim), not SteamVR/PSVR2.
# This script clears those vars in THIS process, restarts Steam clean, and optionally boots SteamVR.
[CmdletBinding()]
param(
    [string]$SteamRoot = 'C:\Program Files (x86)\Steam',
    [switch]$StartSteamVr,
    [switch]$LaunchGame
)

$ErrorActionPreference = 'Stop'
$steamExe = Join-Path $SteamRoot 'steam.exe'
$vrMonitor = Join-Path $SteamRoot 'steamapps\common\SteamVR\bin\win64\vrmonitor.exe'

Remove-Item Env:XR_RUNTIME_JSON -ErrorAction SilentlyContinue
Remove-Item Env:DVR_XRSIM_DIR -ErrorAction SilentlyContinue

Get-Process -Name Singularity -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue

if (Test-Path -LiteralPath $steamExe) {
    Write-Host 'Shutting down Steam (drops inherited xrsim environment from the client)...'
    Start-Process -FilePath $steamExe -ArgumentList @('-shutdown') -Wait -ErrorAction SilentlyContinue | Out-Null
    $deadline = (Get-Date).AddSeconds(25)
    while ((Get-Date) -lt $deadline -and (Get-Process -Name steam -ErrorAction SilentlyContinue)) {
        Start-Sleep -Milliseconds 300
    }
    Start-Sleep -Seconds 2
    # No -silent: a silent client ignores steam:// and -applaunch, so the game
    # looks like it "won't start" after an xrsim test.
    $boot = Start-Process -FilePath $steamExe -WorkingDirectory $SteamRoot -PassThru
    Write-Host "Steam restarted clean (PID $($boot.Id)). OpenXR active runtime = registry SteamVR (not xrsim)."
    Start-Sleep -Seconds 10
}

if ($StartSteamVr -and (Test-Path -LiteralPath $vrMonitor)) {
    if (-not (Get-Process -Name vrserver -ErrorAction SilentlyContinue)) {
        Start-Process -FilePath $vrMonitor | Out-Null
        Write-Host 'SteamVR starting...'
        Start-Sleep -Seconds 8
    }
}

if ($LaunchGame) {
    Start-Process -FilePath 'steam://rungameid/42670' | Out-Null
    Write-Host 'Singularity launch requested. Use headset; do not pass -windowed.'
}

Write-Host @'

Headset checklist:
  1. SteamVR shows PSVR2 / headset ready (not desktop-only).
  2. Launch WITHOUT -windowed (use desktop shortcut or Start-SingularityVR-Headset.ps1).
  3. In-game: BACKSPACE once if still flat (VR mode toggle).
  4. Log should say: OpenXR runtime: SteamVR/Oculus... NOT "dvr-xrsim".

'@
