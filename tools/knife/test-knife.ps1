param([string]$PreparedMesh,[string]$JointCache,[string]$NativeCache,[string]$Golden,[string]$CapturedShaders)
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$out=Join-Path $repo 'build\knife\tests'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$vswhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars=Join-Path $vs 'VC\Auxiliary\Build\vcvarsall.bat'
foreach($test in @('test-hip-knife','test-knife-renderer','test-knife-hand-grip','test-weapon-holster','test-hand-anchor-math','test-desktop-mirror')){
    $source=Join-Path $repo "spikes\view_matrix\tests\$test.cpp"
    $exe=Join-Path $out "$test.exe"
    $obj=Join-Path $out "$test.obj"
    $cmd="`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /MD /Fe:`"$exe`" /Fo:`"$obj`" `"$source`""
    cmd /c $cmd
    if($LASTEXITCODE -ne 0){throw "compile failed $test"}
    if($test -eq 'test-knife-hand-grip' -and $JointCache -and $NativeCache){
        $testArgs=@($JointCache,$NativeCache)
        if($Golden){$testArgs+=$Golden}
        if($Golden -and $CapturedShaders){$testArgs+=0..2 | ForEach-Object {Join-Path $CapturedShaders "$_-VS.bin"}}
        & $exe @testArgs
    }elseif($test -eq 'test-knife-renderer' -and $PreparedMesh){& $exe $PreparedMesh}else{& $exe}
    if($LASTEXITCODE -ne 0){throw "test failed $test"}
}
python (Join-Path $PSScriptRoot 'test_prepare_knife.py')
if($LASTEXITCODE -ne 0){throw 'Python knife tests failed'}
python (Join-Path $PSScriptRoot 'test_fit_hand_grip.py')
if($LASTEXITCODE -ne 0){throw 'Python joint geometry tests failed'}
