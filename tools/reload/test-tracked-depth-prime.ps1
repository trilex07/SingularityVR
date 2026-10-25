$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$out=Join-Path $root 'build/main-seeker-evidence/prime-tests'
New-Item -ItemType Directory -Force $out | Out-Null
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vc=Join-Path $vs 'VC/Auxiliary/Build/vcvarsall.bat'
$src=Join-Path $root 'spikes/view_matrix/tests/test-tracked-depth-prime-d3d9.cpp'
Push-Location $out
try {
    cmd /c "`"$vc`" x86 >nul && cl /nologo /O2 /EHsc /std:c++17 `"$src`" /Fe:test-tracked-depth-prime-d3d9.exe /link d3d9.lib user32.lib && test-tracked-depth-prime-d3d9.exe"
    if($LASTEXITCODE -ne 0){throw 'Tracked depth-prime D3D9/Ex regression failed'}
} finally {Pop-Location}
