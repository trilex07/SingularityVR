# Offline SIDE L1: synthetic CPU + optional locally prepared game-derived data.
param([string]$PreparedDirectory,[string]$SourceTexture,[string]$NativeCache)
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$out=Join-Path $repo 'build\gloves\tests'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$vswhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars=Join-Path $vs 'VC\Auxiliary\Build\vcvarsall.bat'
$tests=@('glove_animation_test','glove_render_state_test','glove_native_test','glove_isolate_test','no_tmd_cooked_palette_test','test-hand-anchor-math')
foreach($test in $tests){
    $source=Join-Path $repo "tools\reference-tests\hands\$test.cpp"
    if($test -eq 'test-hand-anchor-math'){$source=Join-Path $repo "spikes\view_matrix\tests\$test.cpp"}
    $exe=Join-Path $out "$test.exe"
    $obj=Join-Path $out "$test.obj"
    $command="`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /MD /Fe:`"$exe`" /Fo:`"$obj`" `"$source`""
    cmd /c $command
    if($LASTEXITCODE -ne 0){throw "compile failed: $test"}
    if($test -eq 'glove_native_test' -and $NativeCache){& $exe $NativeCache}
    elseif($test -eq 'glove_animation_test' -and $PreparedDirectory){
        & $exe (Join-Path $PreparedDirectory 'glove_left.svrg') (Join-Path $PreparedDirectory 'glove_right.svrg')
    }elseif($test -eq 'glove_render_state_test' -and $PreparedDirectory){
        & $exe $PreparedDirectory
    }else{& $exe}
    if($LASTEXITCODE -ne 0){throw "test failed: $test"}
}
$py=Join-Path $PSScriptRoot 'test_gloves.py'
if($PreparedDirectory -and $SourceTexture){python $py --prepared $PreparedDirectory --texture $SourceTexture}
elseif($PreparedDirectory){python $py --prepared $PreparedDirectory}else{python $py}
if($LASTEXITCODE -ne 0){throw 'Python geometry tests failed'}
