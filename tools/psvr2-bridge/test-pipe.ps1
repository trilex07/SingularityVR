$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Resolve-Path (Join-Path $here '..\..')
$out = Join-Path $root 'deployment\psvr2-bridge-smoke'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$bridgeExe = Join-Path $root 'deployment\psvr2-bridge-x64\SingularityPsvr2Bridge.exe'
if (-not (Test-Path -LiteralPath $bridgeExe -PathType Leaf)) { & (Join-Path $here 'build.ps1') }

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
$bridge = Start-Process -FilePath $bridgeExe -ArgumentList @('"' + $loader + '"',$token) -WindowStyle Hidden -PassThru -RedirectStandardError $stderr
try {
    Start-Sleep -Seconds 2
    if ($bridge.HasExited) { throw "dry bridge exited with code $($bridge.ExitCode)" }
    & $clientExe
    if ($LASTEXITCODE -ne 0) { throw "pipe client smoke failed with code $LASTEXITCODE" }
    Write-Host 'PASS: x86 client authenticated with x64 server and submitted trigger state in dry mode.'
} finally {
    if ($bridge -and -not $bridge.HasExited) { Stop-Process -Id $bridge.Id -Force -ErrorAction SilentlyContinue }
}
Get-Content -LiteralPath $stderr
