# Builds and runs the muzzle-flash per-eye math test. Host architecture, no game and no headset
# needed - pure arithmetic, which is the point: the clip-x rewrite in DrawMuzzleFlashPerEye is
# exactly the kind of thing that otherwise costs a headset run to discover.
#
# The vertex rewrite is duplicated here rather than #included. d3d9.cpp pulls in the whole D3D9
# / OpenXR surface and cannot be compiled standalone, so the alternative to a copy is no test at
# all. If ApplyMuzzleFlashPerEye changes, change it in both places - the test asserts the
# properties, so a drifted copy shows up as a pass here and a seam-straddling flash in the
# headset.
$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) { throw "missing: $vswhere" }
$vsPath = & $vswhere -latest -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) { throw "no VS with x86 C++ toolset found" }
$vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvarsall.bat"
if (-not (Test-Path $vcvars)) { throw "missing: $vcvars" }

$src = Join-Path $here "test_muzzle_per_eye.cpp"
$exe = Join-Path $here "test_muzzle_per_eye.exe"
$obj = Join-Path $here "test_muzzle_per_eye.obj"
# vcvarsall.bat writes to STDERR on this machine while still succeeding. 2>nul avoids turning
# that noise into a terminating error under ErrorActionPreference=Stop.
cmd /c "`"$vcvars`" x86 >nul 2>nul && cl /nologo /EHsc /W4 /Fe:`"$exe`" /Fo:`"$obj`" `"$src`" >nul && `"$exe`""
if ($LASTEXITCODE -ne 0) { throw "muzzle flash per-eye math test FAILED" }
