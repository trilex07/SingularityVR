param([string]$Package=(Join-Path $PSScriptRoot '../../../centurion/final_handoff'),[switch]$SkipGpu,[string[]]$Only=@())
$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/../..").Path
$out=Join-Path $root 'build/centurion-hd'
New-Item -ItemType Directory -Force $out | Out-Null
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vc=Join-Path $vs 'VC/Auxiliary/Build/vcvarsall.bat'
Push-Location $root
try {
 python tools/tests/test-centurion-hd-assets.py $Package "$out/runtime"
 if($LASTEXITCODE){throw 'Actual native Centurion/source geometry failed'}
 . tools/Resolve-OpenXrSdk.ps1
 $xr=Get-OpenXrSdkPaths -RepoRoot $root
 $tests=@('test-test-weapon-grant','test-centurion-animation','test-centurion-visual-followup',
  'test-centurion-tracked-cylinder','test-centurion-physical-polish','test-centurion-reload',
  'test-centurion-cylinder','test-centurion-grip-routing','test-centurion-native-abi',
  'test-next-weapon-reload','test-reload-controls','test-reload-instructions',
  'test-dual-weapon-tmd','test-dual-tmd-weapon-pose','test-dual-tmd-gun-scale',
  'test-tmd-aim','test-tmd-native-adapter','test-measured-aim','test-two-hand-spread',
  'test-locker-vr-loadout-preview','test-locker-vr-loadout-policy','test-locker-vr-loadout-layout',
  'test-sniper-scope','test-weapon-wheel','test-weapon-wheel-layout','test-settings-panel')
 if($Only.Count){$tests=@($tests | Where-Object {$_ -in $Only})}
 foreach($test in $tests){
  if($test -eq 'test-dual-tmd-gun-scale'){& tools/tests/test-dual-tmd-gun-scale.ps1; continue}
  $source=Join-Path $root "spikes/view_matrix/tests/$test.cpp"
  $args=if($test -in @('test-centurion-animation','test-centurion-visual-followup','test-centurion-tracked-cylinder')){"`"$Package/../../dependencies/Singularity/Binaries/vr-assets/reload/centurion/centurion_reload.svca`""}else{''}
  if($test -in @('test-centurion-native-abi','test-dual-tmd-gun-scale')){
   $abi=Join-Path $out 'abi';New-Item -ItemType Directory -Force $abi | Out-Null
   cmd /c "`"$vc`" x86 >nul && cl /nologo /MD /O2 /EHsc /std:c++17 /Ithird_party\minhook\include `"$source`" third_party\minhook\src\buffer.c third_party\minhook\src\hook.c third_party\minhook\src\trampoline.c third_party\minhook\src\hde\hde32.c /Fo:`"$abi/`" /Fe:`"$out/$test.exe`" && `"$out/$test.exe`""
  } else {
   cmd /c "`"$vc`" x86 >nul && cl /nologo /O2 /EHsc /std:c++17 /I`"$($xr.Inc)`" /Ithird_party\minhook\include `"$source`" /Fo:`"$out/$test.obj`" /Fe:`"$out/$test.exe`" && `"$out/$test.exe`" $args"
  }
  if($LASTEXITCODE){throw "Regression failed: $test"}
 }
 if(!$SkipGpu){
  $source=Join-Path $root 'spikes/view_matrix/tests/test-centurion-hd-d3d9.cpp'
  cmd /c "`"$vc`" x86 >nul && cl /nologo /O2 /EHsc /std:c++17 `"$source`" /Fo:`"$out/gpu.obj`" /Fe:`"$out/gpu.exe`" d3d9.lib d3dcompiler.lib user32.lib && `"$out/gpu.exe`" `"$out/runtime/centurion_hd.svct`""
  if($LASTEXITCODE){throw 'D3D9/D3D9Ex actual pixel/state test failed'}
 }
 foreach($source in @('tools/fx/test_tmd_aim_wiring.py','spikes/view_matrix/tests/test-locker-vr-loadout-preview-paths.py','tools/reload/test_locker_vr_loadout_e2e_regression.py')){
  python $source
  if($LASTEXITCODE){throw "Regression failed: $source"}
 }
 & tools/tests/test-session-asset-scope.ps1
} finally {Pop-Location}


