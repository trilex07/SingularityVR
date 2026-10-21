$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$out=Join-Path $repo 'build\nis-review'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$vswhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars=Join-Path $vs 'VC\Auxiliary\Build\vcvarsall.bat'
foreach($test in @('test-nis-upscale','test-nis-bytecode','test-nis-pixels')) {
    $source=Join-Path $repo "spikes\view_matrix\tests\$test.cpp"
    $exe=Join-Path $out "$test.exe"; $obj=Join-Path $out "$test.obj"
    cmd /c "`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W4 /O2 /MD /Fe:`"$exe`" /Fo:`"$obj`" `"$source`""
    if($LASTEXITCODE -ne 0){throw "compile failed $test"}
    & $exe
    if($LASTEXITCODE -ne 0){throw "test failed $test"}
}
python (Join-Path $PSScriptRoot 'test_nis_wiring.py')
if($LASTEXITCODE -ne 0){throw 'NIS/menu wiring failed'}
