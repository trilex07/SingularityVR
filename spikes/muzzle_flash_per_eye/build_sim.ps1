# Builds and runs the muzzle-flash per-eye FULL PIPELINE simulation. Host architecture,
# no game, no headset, no D3D9. Tests detection + splitting + math against synthetic draws.
$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) { throw "missing: $vswhere" }
$vsPath = & $vswhere -latest -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) { throw "no VS with x86 C++ toolset found" }
$vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvarsall.bat"
if (-not (Test-Path $vcvars)) { throw "missing: $vcvars" }

$src = Join-Path $here "sim_muzzle_per_eye.cpp"
$exe = Join-Path $here "sim_muzzle_per_eye.exe"
$obj = Join-Path $here "sim_muzzle_per_eye.obj"
cmd /c "`"$vcvars`" x86 >nul 2>nul && cl /nologo /EHsc /W4 /Fe:`"$exe`" /Fo:`"$obj`" `"$src`" >nul && `"$exe`""
if ($LASTEXITCODE -ne 0) { throw "muzzle flash per-eye SIMULATION test FAILED" }
