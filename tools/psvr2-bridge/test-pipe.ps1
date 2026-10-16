$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Resolve-Path (Join-Path $here '..\..')
$out = Join-Path $root 'deployment\psvr2-bridge-smoke'
New-Item -ItemType Directory -Force -Path $out | Out-Null
& (Join-Path $here 'build.ps1')
$bridgeExe = Join-Path $root 'deployment\psvr2-bridge-x64\SingularityPsvr2Bridge.exe'

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars = Join-Path $vsPath 'VC\Auxiliary\Build\vcvarsall.bat'
$source = Join-Path $here 'pipe_client_smoke.cpp'
$clientExe = Join-Path $out 'psvr2-pipe-client-x86.exe'
$cmd = '"{0}" x86 >nul && cl /nologo /O2 /W4 /EHsc /std:c++17 /I"{1}" /Fe:"{2}" "{3}"' -f $vcvars,$root,$clientExe,$source
$build = cmd /c $cmd
$build | Write-Host
if ($LASTEXITCODE -ne 0) { throw 'x86 pipe smoke client build failed.' }

$token = 'SingularitySmokeToken_20261009_A7F2'
$loader = 'C:\Program Files (x86)\Steam\steamapps\common\PlayStation VR2 App\PSVR2Toolkit\CAPIApps-Windows\psvr2_toolkit_capi_loader.dll'
$stderr = Join-Path $out 'bridge.stderr.log'
if (Test-Path -LiteralPath $stderr) { Remove-Item -LiteralPath $stderr -Force }
$bridge = Start-Process -FilePath $bridgeExe -ArgumentList @('"' + $loader + '"', $token, '--once') -WindowStyle Hidden -PassThru -RedirectStandardError $stderr
try {
    Start-Sleep -Milliseconds 500
    if ($bridge.HasExited) { throw "dry bridge exited early with code $($bridge.ExitCode)" }
    & $clientExe
    if ($LASTEXITCODE -ne 0) { throw "pipe client smoke failed with code $LASTEXITCODE" }
    if (-not $bridge.WaitForExit(3000)) { throw 'bridge did not exit cleanly after --once session' }
    $logText = Get-Content -LiteralPath $stderr -Raw
    Write-Host $logText
    if ($logText -notmatch 'effect=1 hand=0 start=4 end=6 strength=3') { throw 'missing TriggerPistol (4,6,3)' }
    if ($logText -notmatch 'effect=1 hand=0 start=2 end=4 strength=7') { throw 'missing TriggerShotgun (2,4,7)' }
    if ($logText -notmatch 'effect=1 hand=0 start=3 end=5 strength=5') { throw 'missing TriggerRifle (3,5,5)' }
    if ($logText -notmatch 'effect=2 hand=0 start=0 end=0 strength=0 pos=3 amp=7 freq=40') { throw 'missing TriggerRifleFire (3,7,40)' }
    if ($logText -notmatch 'effect=1 hand=0 start=2 end=5 strength=6') { throw 'missing TriggerSniper (2,5,6)' }
    if ($logText -notmatch 'effect=0 hand=0 start=0 end=0 strength=0') { throw 'missing teardown Off state' }
    if ($logText -notmatch 'Client sent Shutdown') { throw 'missing Shutdown message' }
    Write-Host 'PASS: x86 client authenticated with x64 server and verified TriggerPistol, TriggerShotgun, TriggerRifle, TriggerRifleFire, TriggerSniper, Off, and Shutdown.'
} finally {
    if ($bridge -and -not $bridge.HasExited) { Stop-Process -Id $bridge.Id -Force -ErrorAction SilentlyContinue }
}
