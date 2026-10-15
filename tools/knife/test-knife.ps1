param([string]$PreparedMesh)
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$out=Join-Path $repo 'build\knife\tests'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$vswhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars=Join-Path $vs 'VC\Auxiliary\Build\vcvarsall.bat'
foreach($test in @('test-hip-knife','test-knife-renderer','test-weapon-holster','test-hand-anchor-math')){
    $source=Join-Path $repo "spikes\view_matrix\tests\$test.cpp"
    $exe=Join-Path $out "$test.exe"
    $obj=Join-Path $out "$test.obj"
    $cmd="`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /MD /Fe:`"$exe`" /Fo:`"$obj`" `"$source`""
    cmd /c $cmd
    if($LASTEXITCODE -ne 0){throw "compile failed $test"}
    if($test -eq 'test-knife-renderer' -and $PreparedMesh){& $exe $PreparedMesh}else{& $exe}
    if($LASTEXITCODE -ne 0){throw "test failed $test"}
}
python (Join-Path $PSScriptRoot 'test_prepare_knife.py')
if($LASTEXITCODE -ne 0){throw 'Python knife tests failed'}
