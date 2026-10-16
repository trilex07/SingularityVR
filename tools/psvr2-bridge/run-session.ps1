[CmdletBinding()]
param(
    [switch]$Hardware,
    [string]$SteamRoot = 'C:\Program Files (x86)\Steam',
    [int]$AppId = 42670,
    [string]$ToolkitDirectory = 'C:\Program Files (x86)\Steam\steamapps\common\PlayStation VR2 App\PSVR2Toolkit\CAPIApps-Windows',
    [string]$BridgeExe = ''
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if (-not $BridgeExe) { $BridgeExe = Join-Path $PSScriptRoot '..\..\deployment\psvr2-bridge-x64\SingularityPsvr2Bridge.exe' }
$BridgeExe = [System.IO.Path]::GetFullPath($BridgeExe)
$loader = Join-Path $ToolkitDirectory 'psvr2_toolkit_capi_loader.dll'
$steam = Join-Path $SteamRoot 'steam.exe'
if (-not (Test-Path -LiteralPath $BridgeExe -PathType Leaf)) { throw "Bridge executable missing: $BridgeExe. Run build.ps1 first." }
if (-not (Test-Path -LiteralPath $loader -PathType Leaf)) { throw "PSVR2 Toolkit loader missing: $loader" }
if (-not (Test-Path -LiteralPath $steam -PathType Leaf)) { throw "Steam executable missing: $steam" }
if (Get-Process -Name Singularity -ErrorAction SilentlyContinue) { throw 'Close Singularity before starting a bridge session; the session token is read at XR initialization.' }

$stateDir = Join-Path $env:LOCALAPPDATA 'SingularityVR'
New-Item -ItemType Directory -Force -Path $stateDir | Out-Null
$sessionFile = Join-Path $stateDir 'psvr2-trigger.session'
$stderrFile = Join-Path $stateDir 'psvr2-bridge.stderr.log'
$random = [byte[]]::new(24)
$rng = [System.Security.Cryptography.RandomNumberGenerator]::Create()
try { $rng.GetBytes($random) } finally { $rng.Dispose() }
$token = [System.BitConverter]::ToString($random).Replace('-', '')
[System.IO.File]::WriteAllText($sessionFile, $token, [System.Text.Encoding]::ASCII)

$arguments = @('"' + $loader + '"', $token)
if ($Hardware) { $arguments += '--hardware' }
$bridge = Start-Process -FilePath $BridgeExe -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardError $stderrFile
try {
    Start-Sleep -Milliseconds 700
    if ($bridge.HasExited) { throw "PSVR2 bridge exited with code $($bridge.ExitCode). See $stderrFile" }
    Start-Process -FilePath $steam -ArgumentList @('-applaunch', [string]$AppId) -WindowStyle Hidden | Out-Null
    $deadline = (Get-Date).AddMinutes(3)
    $game = $null
    do {
        Start-Sleep -Milliseconds 500
        $game = Get-Process -Name Singularity -ErrorAction SilentlyContinue | Select-Object -First 1
    } while (-not $game -and (Get-Date) -lt $deadline)
    if (-not $game) { throw 'Singularity did not start within three minutes.' }
    Write-Host ("Singularity PID {0}; authenticated x64 bridge PID {1}; hardware={2}" -f $game.Id,$bridge.Id,[bool]$Hardware)
    Wait-Process -Id $game.Id
} finally {
    if ($bridge -and -not $bridge.HasExited) { Stop-Process -Id $bridge.Id -Force -ErrorAction SilentlyContinue }
    Remove-Item -LiteralPath $sessionFile -Force -ErrorAction SilentlyContinue
    [System.Security.Cryptography.CryptographicOperations]::ZeroMemory($random)
}
