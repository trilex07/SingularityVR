param([string]$PreparedRoot='', [switch]$Gpu)
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$out=Join-Path $repo 'build\centurion-reload\tests'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$vswhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars=Join-Path $vs 'VC\Auxiliary\Build\vcvarsall.bat'
foreach($test in @('test-centurion-reload','test-centurion-lifecycle','test-centurion-clip','test-manual-reload','test-hip-knife','test-weapon-holster','test-hand-anchor-math','test-knife-renderer','test-next-weapon-reload','test-next-weapon-animation')){
    $source=Join-Path $repo "spikes\view_matrix\tests\$test.cpp"
    $exe=Join-Path $out "$test.exe"
    $obj=Join-Path $out "$test.obj"
    $command="`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /MD /Fe:`"$exe`" /Fo:`"$obj`" `"$source`""
    cmd /c $command
    if($LASTEXITCODE -ne 0){throw "compile failed: $test"}
    if($test -eq 'test-next-weapon-animation' -and $PreparedRoot){ & $exe $PreparedRoot }
    elseif($test -eq 'test-next-weapon-reload' -and $PreparedRoot){
        & $exe (Join-Path $PreparedRoot 'valkyrie/valkyrie_clip.svrk') (Join-Path $PreparedRoot 'volk/volk_clip.svrk') (Join-Path $PreparedRoot 'kasimov/kasimov_clip.svrk')
    }
    else { & $exe }
    if($LASTEXITCODE -ne 0){throw "test failed: $test"}
}
python (Join-Path $PSScriptRoot 'test_prepare_centurion.py')
if($LASTEXITCODE -ne 0){throw 'Centurion extraction tests failed'}
python (Join-Path $PSScriptRoot 'test_centurion_animation.py')
if($LASTEXITCODE -ne 0){throw 'Centurion animation tests failed'}
python (Join-Path $PSScriptRoot 'test_reload_notifies.py')
if($LASTEXITCODE -ne 0){throw 'Centurion notification tests failed'}
python (Join-Path $PSScriptRoot 'test_analyze_reload_log.py')
if($LASTEXITCODE -ne 0){throw 'Centurion log analysis tests failed'}
python (Join-Path $PSScriptRoot 'test_prepare_next_weapon.py')
if($LASTEXITCODE -ne 0){throw 'Next weapon prototype extraction tests failed'}
python (Join-Path $PSScriptRoot 'test_prepare_weapon_animation.py')
if($LASTEXITCODE -ne 0){throw 'Next weapon animation tests failed'}
if($Gpu){
    if(-not $PreparedRoot){throw 'GPU fixture requires explicit -PreparedRoot'}
    $source=Join-Path $repo 'spikes/view_matrix/tests/test-next-weapon-assets-gpu.cpp'
    $exe=Join-Path $out 'test-next-weapon-assets-gpu.exe'
    $obj=Join-Path $out 'test-next-weapon-assets-gpu.obj'
    cmd /c "`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /MD /Fe:`"$exe`" /Fo:`"$obj`" `"$source`""
    if($LASTEXITCODE -ne 0){throw 'GPU fixture compile failed'}
    & $exe $PreparedRoot
    if($LASTEXITCODE -ne 0){throw 'GPU fixture failed'}
}
