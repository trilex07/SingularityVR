param(
 [string]$Package=(Join-Path $PSScriptRoot '../../../volk_s4/final_handoff'),
 [switch]$SkipGpu
)
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$out=Join-Path $root 'build/volk-hd'
New-Item -ItemType Directory -Force $out | Out-Null
$Package=(Resolve-Path $Package).Path
$vswhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vc=Join-Path $vs 'VC/Auxiliary/Build/vcvarsall.bat'
Push-Location $root
try {
 python tools/tests/test-volk-hd-assets.py $Package "$out/runtime"
 if($LASTEXITCODE){throw 'Real asset geometry regression failed'}
 $source=Join-Path $root 'spikes/view_matrix/tests/test-volk-mechanics.cpp'
 cmd /c "`"$vc`" x86 >nul && cl /nologo /O2 /EHsc /std:c++17 `"$source`" /Fo:`"$out/mechanics.obj`" /Fe:`"$out/mechanics.exe`" && `"$out/mechanics.exe`" `"$Package/source/volk_reload.svwa`""
 if($LASTEXITCODE){throw 'Mechanics regression failed'}
 if(!$SkipGpu){
  $source=Join-Path $root 'spikes/view_matrix/tests/test-volk-hd-d3d9.cpp'
  cmd /c "`"$vc`" x86 >nul && cl /nologo /O2 /EHsc /std:c++17 `"$source`" /Fo:`"$out/gpu.obj`" /Fe:`"$out/gpu.exe`" d3d9.lib d3dcompiler.lib user32.lib && `"$out/gpu.exe`" `"$out/runtime/volk_hd.svvh`""
  if($LASTEXITCODE){throw 'Real D3D9/D3D9Ex pixel/state regression failed'}
 }
 & tools/tests/test-session-asset-scope.ps1
} finally {Pop-Location}
