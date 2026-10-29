$ErrorActionPreference='Stop'
$repo=Resolve-Path (Join-Path $PSScriptRoot '..\..')
$source=Join-Path $repo 'spikes\view_matrix\tests\test-independent-hands-shot-haptics.cpp'
$output=Join-Path $env:TEMP 'test-independent-hands-shot-haptics.exe'
$vswhere='C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
$vsPath=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars=Join-Path $vsPath 'VC\Auxiliary\Build\vcvarsall.bat'
$command='"'+$vcvars+'" x86 >nul && cl /nologo /EHsc /O2 /std:c++17 /Fe:"'+$output+'" "'+$source+'"'
cmd /c $command
if($LASTEXITCODE -ne 0){throw 'test compile failed'}
& $output
if($LASTEXITCODE -ne 0){throw 'test failed'}
Write-Host 'independent hands + shot haptics: PASS'
