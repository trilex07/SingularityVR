$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/..").Path
. (Join-Path $root 'tools/Resolve-OpenXrSdk.ps1')
$xr=Get-OpenXrSdkPaths -RepoRoot $root
$out=Join-Path $root 'build/ui-cleanup/tests'
New-Item -ItemType Directory -Force $out | Out-Null
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vc=Join-Path $vs 'VC/Auxiliary/Build/vcvarsall.bat'
$tests=@('test-vr-legacy-prompts','test-ui-remaster-runtime','test-ui-remaster-switch-combos',
 'test-ui-remaster-d3d9','test-frontend-menu-comfort','test-pause-menu-space',
 'test-settings-panel','test-weapon-wheel','test-weapon-wheel-layout','test-weapon-wheel-catalog',
 'test-menu-submit-policy','test-menu-suppress-policy')
Push-Location $out
try {
 foreach($t in $tests){
  $libs='';if($t -eq 'test-ui-remaster-d3d9'){$libs='/link d3d9.lib user32.lib'}
  $src=Join-Path $root "spikes/view_matrix/tests/$t.cpp"
  cmd /c "`"$vc`" x86 >nul && cl /nologo /O2 /EHsc /std:c++17 /I`"$($xr.Inc)`" /I`"$root/spikes/view_matrix`" `"$src`" /Fe:$t.exe $libs && $t.exe"
  if($LASTEXITCODE -ne 0){throw "FAILED $t"}
 }
}finally{Pop-Location}
