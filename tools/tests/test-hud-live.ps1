param()
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'MSVC x86 toolchain unavailable' }
$vc = Join-Path $vs 'VC\Auxiliary\Build\vcvarsall.bat'
$out = Join-Path $root 'build\hud-live-tests'
New-Item -ItemType Directory -Force -Path $out | Out-Null
foreach ($source in @(
    'spikes/view_matrix/tests/test-health-gesture.cpp',
    'spikes/view_matrix/tests/test-weapon-steady.cpp',
    'spikes/view_matrix/tests/test-ammo-layout.cpp',
    'spikes/view_matrix/tests/test-wrist-equipment-policy.cpp',
    'spikes/view_matrix/tests/test-wrist-equipment-projection.cpp',
    'spikes/view_matrix/tests/test-wrist-equipment-source.cpp',
    'spikes/view_matrix/tests/test-wrist-equipment-depth.cpp',
    'spikes/view_matrix/tests/test-wrist-equipment-placement.cpp',
    'spikes/view_matrix/tests/test-wrist-equipment-integration.cpp'
)) {
    $inputPath = Join-Path $root $source
    $exe = Join-Path $out ([IO.Path]::GetFileNameWithoutExtension($source) + '.exe')
    $command = "`"$vc`" x86 >nul && cl /nologo /O2 /EHsc /std:c++17 `"$inputPath`" /Fe:`"$exe`" && `"$exe`""
    cmd /c $command
    if ($LASTEXITCODE -ne 0) { throw "HUD regression failed: $source" }
}
$source=Join-Path $root 'spikes/view_matrix/tests/test-wrist-equipment-trace-d3d9.cpp'
$exe=Join-Path $out 'test-wrist-equipment-trace-d3d9.exe'
cmd /c "`"$vc`" x86 >nul && cl /nologo /O2 /EHsc /std:c++17 `"$source`" /Fe:`"$exe`" d3d9.lib user32.lib && `"$exe`""
if($LASTEXITCODE -ne 0){throw 'Actual D3D9/D3D9Ex wrist trace regression failed'}
