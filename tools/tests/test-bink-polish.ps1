param([string]$GameRoot='')
$ErrorActionPreference='Stop'
$repo=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$out=Join-Path $repo 'deployment/hands-dubbing/bink-memory-tests'
New-Item -ItemType Directory -Force $out | Out-Null
$vswhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vc=Join-Path $vs 'VC/Auxiliary/Build/vcvarsall.bat'
Push-Location $out
try {
 foreach($name in @('bink-intro-audio','bink-audio-observation','bink-memory-movies')){
  $src=Join-Path $repo "spikes/view_matrix/tests/test-$name.cpp"
  & cmd /c "`"$vc`" x86 >nul && cl /nologo /O2 /EHsc /W3 /MD /std:c++17 /Fe:test-$name.exe `"$src`""
  if($LASTEXITCODE){throw "Compile failed: $name"}
  $test=Join-Path $out "test-$name.exe"
  if($GameRoot -and $name -eq 'bink-memory-movies'){
   & $test (Join-Path $GameRoot 'RvGame/Movies') (Join-Path $GameRoot 'Binaries/binkw32.dll')
  }else{& $test}
  if($LASTEXITCODE){throw "Test failed: $name"}
 }
}finally{Pop-Location}
Write-Output 'PASS x86 O2 Bink selection, memory identity/faults and optional actual offline codec; no gameplay/deployment'
