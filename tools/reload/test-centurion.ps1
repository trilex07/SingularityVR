$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$out=Join-Path $repo 'build\centurion-reload\tests'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$vswhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars=Join-Path $vs 'VC\Auxiliary\Build\vcvarsall.bat'
foreach($test in @('test-centurion-reload','test-manual-reload','test-hip-knife','test-weapon-holster','test-hand-anchor-math','test-knife-renderer')){
    $source=Join-Path $repo "spikes\view_matrix\tests\$test.cpp"
    $exe=Join-Path $out "$test.exe"
    $obj=Join-Path $out "$test.obj"
    $command="`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /MD /Fe:`"$exe`" /Fo:`"$obj`" `"$source`""
    cmd /c $command
    if($LASTEXITCODE -ne 0){throw "compile failed: $test"}
    & $exe
    if($LASTEXITCODE -ne 0){throw "test failed: $test"}
}
python (Join-Path $PSScriptRoot 'test_prepare_centurion.py')
if($LASTEXITCODE -ne 0){throw 'Centurion extraction tests failed'}
