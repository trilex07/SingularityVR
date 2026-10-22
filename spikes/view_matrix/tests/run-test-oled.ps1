# Build and execute deterministic OLED color grading verification test suite
$ErrorActionPreference = 'Stop'

$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Resolve-Path (Join-Path $here "..\..\..")

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvarsall.bat"

Push-Location $here
try {
    Write-Host "Compiling test-oled-color-grading.cpp (x86)..."
    $cmd = "`"$vcvars`" x86 >nul && cl /nologo /O2 /EHsc /W3 /MD /std:c++17 test-oled-color-grading.cpp /Fe:test-oled-color-grading.exe /link d3d11.lib dxgi.lib user32.lib"
    cmd /c $cmd
    if ($LASTEXITCODE -ne 0) { throw "Compilation failed with exit code $LASTEXITCODE" }

    Write-Host "`nRunning test-oled-color-grading.exe..."
    & .\test-oled-color-grading.exe
    if ($LASTEXITCODE -ne 0) { throw "Test execution failed with exit code $LASTEXITCODE" }

} finally {
    Pop-Location
}
