param([string]$Tmd, [string]$Cooked, [string]$Asset)
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$out=Join-Path $repo 'build/gloves/tmd-short-gauntlet/tests'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars=Join-Path $vs 'VC/Auxiliary/Build/vcvarsall.bat'
$source=Join-Path $repo 'spikes/view_matrix/tests/test-tmd-short-gauntlet.cpp'
$exe=Join-Path $out 'test-tmd-short-gauntlet.exe'
cmd /c "`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W3 /O2 /MD /Fe:`"$exe`" /Fo:`"$out/gauntlet.obj`" `"$source`""
if($LASTEXITCODE -ne 0){throw 'TMD gauntlet C++ compile failed'}
& $exe
if($LASTEXITCODE -ne 0){throw 'TMD gauntlet C++ test failed'}
if($Tmd -and $Cooked -and $Asset){
    py -3 (Join-Path $PSScriptRoot 'test_tmd_short_gauntlet.py') --tmd $Tmd --cooked $Cooked --asset $Asset
    if($LASTEXITCODE -ne 0){throw 'TMD gauntlet private-source test failed'}
}
