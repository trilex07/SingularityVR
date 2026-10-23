$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/../..").Path
$out=Join-Path $root 'build/dual-tmd-gun-scale'
New-Item -ItemType Directory -Force $out | Out-Null
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vc=Join-Path $vs 'VC/Auxiliary/Build/vcvarsall.bat'
$mh=Join-Path $root 'third_party/minhook'
$src=Join-Path $root 'spikes/view_matrix/tests/test-dual-tmd-gun-scale.cpp'
$c=@('buffer.c','hook.c','trampoline.c','hde/hde32.c') | ForEach-Object { '"'+(Join-Path "$mh/src" $_)+'"' }
Push-Location $out
try {
    cmd /c "`"$vc`" x86 >nul && cl /nologo /O2 /EHsc /std:c++17 /I`"$mh/include`" `"$src`" $($c -join ' ') /Fe:test-dual-tmd-gun-scale.exe /link /BASE:0x400000 /DYNAMICBASE:NO /MAP:test-dual-tmd-gun-scale.map && test-dual-tmd-gun-scale.exe"
    if($LASTEXITCODE -ne 0){throw 'Gun-scale real x86 adapter regression failed'}
} finally {Pop-Location}
