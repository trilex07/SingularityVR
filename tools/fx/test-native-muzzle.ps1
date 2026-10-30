$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$out=Join-Path $repo 'build\native-muzzle\tests'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$vswhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars=Join-Path $vs 'VC\Auxiliary\Build\vcvarsall.bat'
foreach($test in @('test-gore-policy','test-tmd-aim','test-tmd-native-adapter','test-body-equipment','test-measured-aim','test-weapon-steady','test-native-registry','test-native-muzzle','test-native-muzzle-code-capture','test-native-muzzle-spawn-abi','test-rifle-native-origin','test-scope-pip','test-shadow-world-receiver','test-shadow-first-person-receiver','test-vfx-eye-scale','test-hand-anchor-math','test-weapon-holster')){
    $source=Join-Path $repo "spikes\view_matrix\tests\$test.cpp"
    if(!(Test-Path -LiteralPath $source)){throw "Missing test $source"}
    $exe=Join-Path $out "$test.exe"; $obj=Join-Path $out "$test.obj"
    cmd /c "`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /MD /Fe:`"$exe`" /Fo:`"$obj`" `"$source`""
    if($LASTEXITCODE -ne 0){throw "compile failed $test"}
    & $exe
    if($LASTEXITCODE -ne 0){throw "test failed $test"}
}
python (Join-Path $repo 'tools\tests\test-shadow-trace-analysis.py')
if($LASTEXITCODE -ne 0){throw 'trace analysis regression failed'}
python (Join-Path $PSScriptRoot 'test_native_muzzle_analysis.py')
if($LASTEXITCODE -ne 0){throw 'native muzzle analysis tests failed'}
python (Join-Path $PSScriptRoot 'test_native_particle_transforms.py')
if($LASTEXITCODE -ne 0){throw 'native particle transform tests failed'}
python (Join-Path $PSScriptRoot 'test_rifle_scope_backend.py')
if($LASTEXITCODE -ne 0){throw 'rifle/scope backend guard tests failed'}
python (Join-Path $PSScriptRoot 'test_measured_aim_wiring.py')
if($LASTEXITCODE -ne 0){throw 'measured aim/equipment wiring failed'}
python (Join-Path $PSScriptRoot 'test_deadlock_backend_audit.py')
if($LASTEXITCODE -ne 0){throw 'Deadlock constructor identity tests failed'}
python (Join-Path $PSScriptRoot 'test_tmd_aim_wiring.py')
if($LASTEXITCODE -ne 0){throw 'TMD getter scope wiring failed'}
