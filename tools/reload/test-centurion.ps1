param([string]$NativeEvidence,[string]$AnimationCache,[string]$ShellAssets,[string]$NativeVertices,[string]$RefillEvidence,[string]$PreparedRoot='', [string]$NativeSpaceRoot='', [switch]$Gpu)
$ErrorActionPreference='Stop'
# unittest writes its progress and summary to STDERR. With
# $ErrorActionPreference='Stop' a native command that writes to stderr aborts the
# script before the exit code is ever read, so the streams are merged here and
# the exit code is checked explicitly.
function Invoke-PythonCheck([string]$Script,[string]$Label){
  $ErrorActionPreference='Continue'
  $output = & python $Script 2>&1
  $code=$LASTEXITCODE
  $ErrorActionPreference='Stop'
  $output | ForEach-Object { Write-Host $_ }
  if($code -ne 0){throw $Label}
}
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$out=Join-Path $repo 'build\centurion-reload\tests'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$vswhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars=Join-Path $vs 'VC\Auxiliary\Build\vcvarsall.bat'
. (Join-Path $repo 'tools/Resolve-OpenXrSdk.ps1')
$xr=Get-OpenXrSdkPaths -RepoRoot $repo
$panelSource=Join-Path $repo 'spikes/view_matrix/tests/test-settings-panel.cpp'
$panelExe=Join-Path $out 'test-settings-panel.exe'
$panelObj=Join-Path $out 'test-settings-panel.obj'
cmd /c "`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W3 /O2 /MD /I`"$($xr.Inc)`" /Fe:`"$panelExe`" /Fo:`"$panelObj`" `"$panelSource`""
if($LASTEXITCODE -ne 0){throw 'Settings panel compile failed'}
& $panelExe
if($LASTEXITCODE -ne 0){throw 'Settings panel math/input failed'}
Invoke-PythonCheck (Join-Path $PSScriptRoot 'test_settings_panel.py') 'Settings panel wiring failed'
$instructionSource=Join-Path $repo 'spikes/view_matrix/tests/test-reload-instructions.cpp'
$instructionExe=Join-Path $out 'test-reload-instructions.exe'
$instructionObj=Join-Path $out 'test-reload-instructions.obj'
cmd /c "`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /MD /Fe:`"$instructionExe`" /Fo:`"$instructionObj`" `"$instructionSource`""
if($LASTEXITCODE -ne 0){throw 'Reload instructions compile failed'}
& $instructionExe
if($LASTEXITCODE -ne 0){throw 'Four-weapon PL/EN instructions failed'}
foreach($visualTest in @('test-sniper-scope','test-wheel-grip-followup','test-weapon-wheel','test-weapon-wheel-layout','test-weapon-wheel-catalog')){
    $source=Join-Path $repo "spikes/view_matrix/tests/$visualTest.cpp"
    $exe=Join-Path $out "$visualTest.exe"
    $obj=Join-Path $out "$visualTest.obj"
    cmd /c "`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /MD /Fe:`"$exe`" /Fo:`"$obj`" `"$source`""
    if($LASTEXITCODE -ne 0){throw "Visual followup compile failed: $visualTest"}
    & $exe
    if($LASTEXITCODE -ne 0){throw "Visual followup failed: $visualTest"}
}
foreach($test in @('test-centurion-reload','test-centurion-lifecycle','test-centurion-grip-routing','test-reload-controls','test-centurion-cylinder','test-centurion-native','test-centurion-refill','test-centurion-clip','test-centurion-animation','test-centurion-visual-followup','test-centurion-tracked-cylinder','test-centurion-physical-polish','test-centurion-shells','test-centurion-shoulder','test-manual-reload','test-hip-knife','test-hud-reseat','test-weapon-holster','test-hand-anchor-math','test-knife-renderer','test-next-weapon-reload','test-next-weapon-animation','test-next-weapon-runtime','test-next-weapon-refill','test-next-weapon-refill-visual','test-weapon-carry-ray')){
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
if($NativeSpaceRoot){
    if(-not $PreparedRoot){throw 'Native space fixture requires original -PreparedRoot'}
    $source=Join-Path $repo 'spikes/view_matrix/tests/test-next-weapon-native-space.cpp'
    $exe=Join-Path $out 'test-next-weapon-native-space.exe'
    $obj=Join-Path $out 'test-next-weapon-native-space.obj'
    cmd /c "`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /MD /Fe:`"$exe`" /Fo:`"$obj`" `"$source`""
    if($LASTEXITCODE -ne 0){throw 'Native space fixture compile failed'}
    & $exe $PreparedRoot $NativeSpaceRoot
    if($LASTEXITCODE -ne 0){throw 'Original native coordinate/mesh/replay proof failed'}
}
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
Invoke-PythonCheck (Join-Path $PSScriptRoot 'test_prepare_centurion.py') 'Centurion extraction tests failed'
Invoke-PythonCheck (Join-Path $PSScriptRoot 'test_centurion_animation.py') 'Centurion animation tests failed'
Invoke-PythonCheck (Join-Path $PSScriptRoot 'test_reload_notifies.py') 'Centurion notification tests failed'
Invoke-PythonCheck (Join-Path $PSScriptRoot 'test_analyze_reload_log.py') 'Centurion log analysis tests failed'
Invoke-PythonCheck (Join-Path $PSScriptRoot 'test_prepare_next_weapon.py') 'Next weapon prototype extraction tests failed'
Invoke-PythonCheck (Join-Path $PSScriptRoot 'test_prepare_weapon_animation.py') 'Next weapon animation tests failed'
Invoke-PythonCheck (Join-Path $PSScriptRoot 'test_audit_next_weapon_space.py') 'Native cooked coordinate audit guards failed'
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
Invoke-PythonCheck (Join-Path $PSScriptRoot 'test_inspect_native_reload.py') 'Native reload offline mapper tests failed'
Invoke-PythonCheck (Join-Path $PSScriptRoot 'test_refill_only.py') 'Refill-only evidence/wiring tests failed'
Invoke-PythonCheck (Join-Path $PSScriptRoot 'test_cylinder_animation.py') 'Cylinder animation cache tests failed'
Invoke-PythonCheck (Join-Path $PSScriptRoot 'test_prepare_centurion_shells.py') 'Original shell extraction tests failed'
Invoke-PythonCheck (Join-Path $PSScriptRoot 'test_tracked_cylinder_wiring.py') 'Tracked cylinder wiring guards failed'
Invoke-PythonCheck (Join-Path $PSScriptRoot 'test_native_space.py') 'Native space evidence guards failed'
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
Invoke-PythonCheck (Join-Path $PSScriptRoot 'test_next_weapon_native.py') 'Three-weapon inherited method checks failed'
Invoke-PythonCheck (Join-Path $PSScriptRoot 'test_next_refill.py') 'Three-weapon refill-only inheritance/wiring checks failed'
Invoke-PythonCheck (Join-Path $PSScriptRoot 'test_runtime_candidate.py') 'Offline candidate safety/provenance checks failed'
