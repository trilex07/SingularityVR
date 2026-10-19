param([string]$NativeEvidence,[string]$AnimationCache,[string]$ShellAssets,[string]$NativeVertices,[string]$RefillEvidence,[string]$PreparedRoot='', [switch]$Gpu)
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$out=Join-Path $repo 'build\centurion-reload\tests'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$vswhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars=Join-Path $vs 'VC\Auxiliary\Build\vcvarsall.bat'
$instructionSource=Join-Path $repo 'spikes/view_matrix/tests/test-reload-instructions.cpp'
$instructionExe=Join-Path $out 'test-reload-instructions.exe'
$instructionObj=Join-Path $out 'test-reload-instructions.obj'
cmd /c "`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /MD /Fe:`"$instructionExe`" /Fo:`"$instructionObj`" `"$instructionSource`""
if($LASTEXITCODE -ne 0){throw 'Reload instructions compile failed'}
& $instructionExe
if($LASTEXITCODE -ne 0){throw 'Four-weapon PL/EN instructions failed'}
foreach($test in @('test-centurion-reload','test-centurion-lifecycle','test-centurion-grip-routing','test-reload-controls','test-centurion-cylinder','test-centurion-native','test-centurion-refill','test-centurion-clip','test-centurion-animation','test-centurion-visual-followup','test-centurion-tracked-cylinder','test-centurion-physical-polish','test-centurion-shells','test-centurion-shoulder','test-manual-reload','test-hip-knife','test-hud-reseat','test-weapon-holster','test-hand-anchor-math','test-knife-renderer','test-next-weapon-reload','test-next-weapon-animation','test-next-weapon-runtime','test-next-weapon-refill','test-next-weapon-refill-visual')){
    $source=Join-Path $repo "spikes\view_matrix\tests\$test.cpp"
    $exe=Join-Path $out "$test.exe"
    $obj=Join-Path $out "$test.obj"
    $command="`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /MD /Fe:`"$exe`" /Fo:`"$obj`" `"$source`""
    cmd /c $command
    if($LASTEXITCODE -ne 0){throw "compile failed: $test"}
    if($test -in @('test-next-weapon-animation','test-next-weapon-runtime','test-next-weapon-refill-visual') -and $PreparedRoot){ & $exe $PreparedRoot }
    elseif($test -eq 'test-next-weapon-runtime') { throw 'Runtime fixture requires explicit -PreparedRoot (private originals)' }
    elseif($test -eq 'test-next-weapon-reload' -and $PreparedRoot){
        & $exe (Join-Path $PreparedRoot 'valkyrie/valkyrie_clip.svrk') (Join-Path $PreparedRoot 'volk/volk_clip.svrk') (Join-Path $PreparedRoot 'kasimov/kasimov_clip.svrk')
    }
    elseif($test -eq 'test-next-weapon-refill' -and $RefillEvidence){
        $evidence=Get-Content -LiteralPath $RefillEvidence -Raw | ConvertFrom-Json
        if((Get-FileHash -LiteralPath $evidence.text_path -Algorithm SHA256).Hash -ne $evidence.text_sha256){throw 'Archived refill text changed'}
        & $exe $evidence.text_path $evidence.text_base $evidence.methods[0].method $evidence.methods[1].method $evidence.methods[2].method $evidence.methods[3].method $evidence.methods[0].wrapper $evidence.methods[1].wrapper $evidence.methods[2].wrapper $evidence.methods[3].wrapper
    }
    elseif($test -eq 'test-centurion-native' -and $NativeEvidence){
        $evidence=Get-Content -LiteralPath $NativeEvidence -Raw | ConvertFrom-Json
        if((Get-FileHash -LiteralPath $evidence.text_path -Algorithm SHA256).Hash -ne $evidence.text_sha256){throw 'Archived text changed after offline mapping'}
        & $exe $evidence.text_path $evidence.text_base $evidence.registration.wrapper $evidence.method
    }elseif($test -eq 'test-centurion-refill' -and $RefillEvidence){
        $evidence=Get-Content -LiteralPath $RefillEvidence -Raw | ConvertFrom-Json
        if((Get-FileHash -LiteralPath $evidence.text_path -Algorithm SHA256).Hash -ne $evidence.text_sha256){throw 'Archived refill evidence changed'}
        & $exe $evidence.text_path $evidence.text_base $evidence.methods[0].method $evidence.methods[1].method $evidence.methods[2].method $evidence.methods[3].method $evidence.methods[0].wrapper $evidence.methods[1].wrapper $evidence.methods[2].wrapper $evidence.methods[3].wrapper
    }elseif($test -in @('test-centurion-animation','test-centurion-visual-followup','test-centurion-tracked-cylinder') -and $AnimationCache){& $exe $AnimationCache}else{& $exe}
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
    $nextAbiSource=Join-Path $repo 'spikes/view_matrix/tests/test-next-weapon-native-abi.cpp'
    $nextAbiExe=Join-Path $out 'test-next-weapon-native-abi.exe'
    $command="`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /MD /I`"$mh\include`" /Fe:`"$nextAbiExe`" `"$nextAbiSource`" buffer.obj hook.obj trampoline.obj hde32.obj"
    cmd /c $command
    if($LASTEXITCODE -ne 0){throw 'three-weapon native ABI harness compile failed'}
    & $nextAbiExe
    if($LASTEXITCODE -ne 0){throw 'three-weapon native ABI harness failed'}
}finally{Pop-Location}
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
    $source=Join-Path $repo 'spikes/view_matrix/tests/test-next-weapon-native-spare-gpu.cpp'
    $exe=Join-Path $out 'test-next-weapon-native-spare-gpu.exe'
    $obj=Join-Path $out 'test-next-weapon-native-spare-gpu.obj'
    cmd /c "`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /MD /Fe:`"$exe`" /Fo:`"$obj`" `"$source`""
    if($LASTEXITCODE -ne 0){throw 'Native magazine GPU fixture compile failed'}
    & $exe
    if($LASTEXITCODE -ne 0){throw 'Native magazine GPU fixture failed'}
}
python (Join-Path $PSScriptRoot 'test_inspect_native_reload.py')
if($LASTEXITCODE -ne 0){throw 'Native reload offline mapper tests failed'}
python (Join-Path $PSScriptRoot 'test_refill_only.py')
if($LASTEXITCODE -ne 0){throw 'Refill-only evidence/wiring tests failed'}
python (Join-Path $PSScriptRoot 'test_cylinder_animation.py')
if($LASTEXITCODE -ne 0){throw 'Cylinder animation cache tests failed'}
python (Join-Path $PSScriptRoot 'test_prepare_centurion_shells.py')
if($LASTEXITCODE -ne 0){throw 'Original shell extraction tests failed'}
python (Join-Path $PSScriptRoot 'test_tracked_cylinder_wiring.py')
if($LASTEXITCODE -ne 0){throw 'Tracked cylinder wiring guards failed'}
python (Join-Path $PSScriptRoot 'test_native_space.py')
if($LASTEXITCODE -ne 0){throw 'Native space evidence guards failed'}
if($NativeVertices){
    if(!$AnimationCache){throw 'Native vertex oracle requires original animation cache'}
    $source=Join-Path $repo 'spikes/view_matrix/tests/test-centurion-native-space-gpu.cpp'
    $exe=Join-Path $out 'test-centurion-native-space-gpu.exe'
    $obj=Join-Path $out 'test-centurion-native-space-gpu.obj'
    cmd /c "`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /MD /Fe:`"$exe`" /Fo:`"$obj`" `"$source`""
    if($LASTEXITCODE -ne 0){throw 'Native vertex oracle compile failed'}
    & $exe $AnimationCache $NativeVertices
    if($LASTEXITCODE -ne 0){throw 'Native vertex oracle failed'}
}
# Optional own-process D3D resource test: never loads the proxy or game.
$source=Join-Path $repo 'spikes/view_matrix/tests/test-centurion-animation-ib.cpp'
$exe=Join-Path $out 'test-centurion-animation-ib.exe'
$obj=Join-Path $out 'test-centurion-animation-ib.obj'
cmd /c "`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /MD /Fe:`"$exe`" /Fo:`"$obj`" `"$source`""
if($LASTEXITCODE -ne 0){throw 'IB witness fixture compile failed'}
& $exe
if($LASTEXITCODE -ne 0){throw 'IB witness fixture failed'}
$source=Join-Path $repo 'spikes/view_matrix/tests/test-centurion-native-spare-gpu.cpp'
$exe=Join-Path $out 'test-centurion-native-spare-gpu.exe'
$obj=Join-Path $out 'test-centurion-native-spare-gpu.obj'
cmd /c "`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /MD /Fe:`"$exe`" /Fo:`"$obj`" `"$source`""
if($LASTEXITCODE -ne 0){throw 'Native spare GPU fixture compile failed'}
& $exe
if($LASTEXITCODE -ne 0){throw 'Native spare GPU fixture failed'}
if($ShellAssets){
    $source=Join-Path $repo 'spikes/view_matrix/tests/test-centurion-shells-gpu.cpp'
    $exe=Join-Path $out 'test-centurion-shells-gpu.exe'
    $obj=Join-Path $out 'test-centurion-shells-gpu.obj'
    cmd /c "`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /MD /Fe:`"$exe`" /Fo:`"$obj`" `"$source`""
    if($LASTEXITCODE -ne 0){throw 'Shell GPU fixture compile failed'}
    & $exe $ShellAssets
    if($LASTEXITCODE -ne 0){throw 'Shell GPU fixture failed'}
}
python (Join-Path $PSScriptRoot 'test_next_weapon_native.py')
if($LASTEXITCODE -ne 0){throw 'Three-weapon inherited method checks failed'}
python (Join-Path $PSScriptRoot 'test_next_refill.py')
if($LASTEXITCODE -ne 0){throw 'Three-weapon refill-only inheritance/wiring checks failed'}
python (Join-Path $PSScriptRoot 'test_runtime_candidate.py')
if($LASTEXITCODE -ne 0){throw 'Offline candidate safety/provenance checks failed'}
