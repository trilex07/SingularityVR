$ErrorActionPreference = 'Stop'
$repo = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$out = Join-Path $repo 'build\hud\tests'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvarsall.bat'

$test = 'test-diegetic-hud'
$source = Join-Path $repo "spikes\view_matrix\tests\$test.cpp"
$exe = Join-Path $out "$test.exe"
$obj = Join-Path $out "$test.obj"
$cmd = "`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /MD /I`"$repo\spikes\view_matrix`" /Fe:`"$exe`" /Fo:`"$obj`" `"$source`""
cmd /c $cmd
if ($LASTEXITCODE -ne 0) { throw "compile failed $test" }

& $exe
if ($LASTEXITCODE -ne 0) { throw "test failed $test" }
Write-Host "=== Diegetic HUD C++ unit tests completed successfully ===" -ForegroundColor Green
