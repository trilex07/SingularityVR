$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$out=Join-Path $root 'build/main-seeker-evidence/tmd-socket-tests'
New-Item -ItemType Directory -Force $out | Out-Null
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vc=Join-Path $vs 'VC/Auxiliary/Build/vcvarsall.bat'
$src=Join-Path $root 'spikes/view_matrix/tests/test-tmd-muzzle-socket.cpp'
Push-Location $out
try {
    cmd /c "`"$vc`" x86 >nul && cl /nologo /O2 /EHsc /std:c++17 /W4 /WX `"$src`" /Fe:test-tmd-muzzle-socket.exe && test-tmd-muzzle-socket.exe"
    if($LASTEXITCODE -ne 0){throw 'TMD socket math/native adapter regression failed'}
}finally{Pop-Location}
