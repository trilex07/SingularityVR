$ErrorActionPreference = 'Stop'
$root = Resolve-Path (Join-Path $PSScriptRoot '..\..\..')
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars = Join-Path $vsPath 'VC\Auxiliary\Build\vcvarsall.bat'
$src = Join-Path $PSScriptRoot 'test-microstutter-probe.cpp'
$output = Join-Path $PSScriptRoot 'test-microstutter-probe.exe'
$cmd = "`"$vcvars`" x86 >nul && cl /nologo /EHsc /O2 /std:c++17 /Fe:`"$output`" `"$src`""
cmd /c $cmd
if ($LASTEXITCODE -ne 0) { throw 'microstutter probe compilation failed' }
$dummyLog = Join-Path $PSScriptRoot 'microstutter-test.log'
& $output $dummyLog
if ($LASTEXITCODE -ne 0) { throw 'microstutter probe test failed' }
Write-Host 'microstutter probe PASS'
foreach ($name in @('test-hotpath-sampler', 'test-hotpath-sampler-scaling', 'test-native-gamepad-enum-probe')) {
    $regSrc = Join-Path $PSScriptRoot ($name + '.cpp')
    $regExe = Join-Path $PSScriptRoot ($name + '.exe')
    $regCmd = "`"$vcvars`" x86 >nul && cl /nologo /EHsc /O2 /std:c++17 /Fe:`"$regExe`" `"$regSrc`""
    cmd /c $regCmd
    if ($LASTEXITCODE -ne 0) { throw "$name compilation failed" }
    & $regExe
    if ($LASTEXITCODE -ne 0) { throw "$name regression failed" }
}
