$ErrorActionPreference='Stop'
$repo=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$evidence=Join-Path $repo 'deployment/hands-dubbing/post-opening/tests'
New-Item -ItemType Directory -Path $evidence -Force | Out-Null
$vswhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars=Join-Path $vsPath 'VC/Auxiliary/Build/vcvarsall.bat'
Push-Location $evidence
try {
 foreach($name in @('native-voice-ownership','hand-anchor-probe','hands-only','hand-anchor-math','menu-submit-policy','cutscene-mono','cinema-world-screen','foreground-boundary','bink-intro-audio','ui-asset-witness')) {
  $source=Join-Path $repo "spikes/view_matrix/tests/test-$name.cpp"
  $extra=if($name -eq 'native-voice-ownership'){ '"'+(Join-Path $repo 'spikes/view_matrix/polish_localization.cpp')+'"' }else{''}
  $cmd='"'+$vcvars+'" x86 >nul && cl /nologo /O2 /EHsc /W3 /MD /std:c++17 /Fe:test-'+$name+'.exe "'+$source+'" '+$extra
  & cmd /c $cmd *> "compile-$name.log"
  if($LASTEXITCODE -ne 0){throw "Compilation failed: $name (see owned test log)"}
  & (Join-Path $evidence "test-$name.exe")
  if($LASTEXITCODE -ne 0){throw "Test failed: $name"}
 }
} finally {Pop-Location}
& (Join-Path $repo 'spikes/view_matrix/tests/validate-runtime-policy.ps1')
Write-Output 'PASS: ten optimized x86 regression fixtures; no Steam launch/install'
