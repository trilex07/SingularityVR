# Builds and runs the ShadowMode=3 tests (x86 PE32, /O2 /std:c++17 /W4 /WX).
$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath  = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars  = Join-Path $vsPath "VC\Auxiliary\Build\vcvarsall.bat"

Push-Location $here
try {
    cmd /c "`"$vcvars`" x86 && cd /d `"$here`" && cl /nologo /EHsc /O2 /std:c++17 /W4 /WX /Fe:shadow_mode3_test.exe shadow_mode3_test.cpp"
    if ($LASTEXITCODE -ne 0) { throw "build failed" }
    & (Join-Path $here "shadow_mode3_test.exe")
    if ($LASTEXITCODE -ne 0) { throw "tests failed" }
    $out = cmd /c "`"$vcvars`" x86 >nul 2>nul && dumpbin /nologo /headers shadow_mode3_test.exe"
    $out | Select-String -Pattern "machine \(x86\)|32 bit word" | ForEach-Object { $_.Line.Trim() }
} finally {
    Pop-Location
}
