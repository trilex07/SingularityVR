param([string]$NativeEvidence,[string]$AnimationCache)
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$out=Join-Path $repo 'build\centurion-reload\tests'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$vswhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars=Join-Path $vs 'VC\Auxiliary\Build\vcvarsall.bat'
foreach($test in @('test-centurion-reload','test-centurion-lifecycle','test-centurion-grip-routing','test-reload-controls','test-centurion-cylinder','test-centurion-native','test-centurion-clip','test-centurion-animation','test-manual-reload','test-hip-knife','test-weapon-holster','test-hand-anchor-math','test-knife-renderer')){
    $source=Join-Path $repo "spikes\view_matrix\tests\$test.cpp"
    $exe=Join-Path $out "$test.exe"
    $obj=Join-Path $out "$test.obj"
    $command="`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /MD /Fe:`"$exe`" /Fo:`"$obj`" `"$source`""
    cmd /c $command
    if($LASTEXITCODE -ne 0){throw "compile failed: $test"}
    if($test -eq 'test-centurion-native' -and $NativeEvidence){
        $evidence=Get-Content -LiteralPath $NativeEvidence -Raw | ConvertFrom-Json
        if((Get-FileHash -LiteralPath $evidence.text_path -Algorithm SHA256).Hash -ne $evidence.text_sha256){throw 'Archived text changed after offline mapping'}
        & $exe $evidence.text_path $evidence.text_base $evidence.registration.wrapper $evidence.method
    }elseif($test -eq 'test-centurion-animation' -and $AnimationCache){& $exe $AnimationCache}else{& $exe}
    if($LASTEXITCODE -ne 0){throw "test failed: $test"}
}
# This detours only the fake method in its OWN test process, never the game.
$abiSource=Join-Path $repo 'spikes\view_matrix\tests\test-centurion-native-abi.cpp'
$abiExe=Join-Path $out 'test-centurion-native-abi.exe'
$mh=Join-Path $repo 'third_party\minhook'
$mhSources=@('buffer.c','hook.c','trampoline.c','hde\hde32.c') | ForEach-Object {"`"$(Join-Path $mh "src\$_")`""}
Push-Location $out
try{
    # Preserve production's warning policy for unchanged upstream C sources;
    # our harness/policy still compile under /W4 /WX independently.
    $command="`"$vcvars`" x86 >nul && cl /nologo /W3 /O2 /MD /c $($mhSources -join ' ')"
    cmd /c $command
    if($LASTEXITCODE -ne 0){throw 'MinHook ABI test dependency compile failed'}
    $command="`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /MD /I`"$mh\include`" /Fe:`"$abiExe`" `"$abiSource`" buffer.obj hook.obj trampoline.obj hde32.obj"
    cmd /c $command
    if($LASTEXITCODE -ne 0){throw 'native ABI harness compile failed'}
    & $abiExe
    if($LASTEXITCODE -ne 0){throw 'native ABI harness failed'}
}finally{Pop-Location}
python (Join-Path $PSScriptRoot 'test_prepare_centurion.py')
if($LASTEXITCODE -ne 0){throw 'Centurion extraction tests failed'}
python (Join-Path $PSScriptRoot 'test_centurion_animation.py')
if($LASTEXITCODE -ne 0){throw 'Centurion animation tests failed'}
python (Join-Path $PSScriptRoot 'test_reload_notifies.py')
if($LASTEXITCODE -ne 0){throw 'Centurion notification tests failed'}
python (Join-Path $PSScriptRoot 'test_analyze_reload_log.py')
if($LASTEXITCODE -ne 0){throw 'Centurion log analysis tests failed'}
python (Join-Path $PSScriptRoot 'test_inspect_native_reload.py')
if($LASTEXITCODE -ne 0){throw 'Native reload offline mapper tests failed'}
python (Join-Path $PSScriptRoot 'test_cylinder_animation.py')
if($LASTEXITCODE -ne 0){throw 'Cylinder animation cache tests failed'}
# Optional own-process D3D resource test: never loads the proxy or game.
$source=Join-Path $repo 'spikes/view_matrix/tests/test-centurion-animation-ib.cpp'
$exe=Join-Path $out 'test-centurion-animation-ib.exe'
$obj=Join-Path $out 'test-centurion-animation-ib.obj'
cmd /c "`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /MD /Fe:`"$exe`" /Fo:`"$obj`" `"$source`""
if($LASTEXITCODE -ne 0){throw 'IB witness fixture compile failed'}
& $exe
if($LASTEXITCODE -ne 0){throw 'IB witness fixture failed'}
