$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '../../../')).Path
$out = Join-Path $root 'build/shadow-tests'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$source = Join-Path $PSScriptRoot 'shadow_policy_test.cpp'
$exe = Join-Path $out 'shadow_policy_test.exe'

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
if (!(Test-Path $vswhere)) { throw "vswhere not found: $vswhere" }
$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vsPath) { throw 'MSVC installation not found' }
$vcvars = Join-Path $vsPath 'VC/Auxiliary/Build/vcvarsall.bat'

$compile = "`"$vcvars`" x86 && cl /nologo /EHsc /W4 /WX /O2 /std:c++17 /I`"$root/spikes/view_matrix`" `"$source`" /Fe:`"$exe`""
cmd /c $compile
if ($LASTEXITCODE -ne 0) { throw "shadow policy compile failed ($LASTEXITCODE)" }

& $exe
if ($LASTEXITCODE -ne 0) { throw "shadow policy tests failed ($LASTEXITCODE)" }
Write-Host "shadow policy tests passed: $exe"
