param([string]$Cache)
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$out=Join-Path $repo 'build/gloves/cuff-repair/tests'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars=Join-Path $vs 'VC/Auxiliary/Build/vcvarsall.bat'
$source=Join-Path $repo 'spikes/view_matrix/tests/test-native-glove-cuffs.cpp'
$exe=Join-Path $out 'test-native-glove-cuffs.exe'
cmd /c "`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /MD /Fe:`"$exe`" /Fo:`"$out/cuffs.obj`" `"$source`""
if($LASTEXITCODE -ne 0){throw 'cuff C++ compile failed'}
if($Cache){& $exe $Cache}else{& $exe}
if($LASTEXITCODE -ne 0){throw 'cuff C++ test failed'}
python (Join-Path $PSScriptRoot 'test_cuffs.py')
if($LASTEXITCODE -ne 0){throw 'cuff synthetic geometry test failed'}
