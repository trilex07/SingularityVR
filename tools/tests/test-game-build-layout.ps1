param(
    [Parameter(Mandatory)][string]$EnglishExe,
    [Parameter(Mandatory)][string]$GermanExe,
    [string]$ProtectedRetailExe
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot/../..").Path
$english = (Resolve-Path -LiteralPath $EnglishExe).Path
$german = (Resolve-Path -LiteralPath $GermanExe).Path
$retail = if ($ProtectedRetailExe) { (Resolve-Path -LiteralPath $ProtectedRetailExe).Path }
$out = Join-Path $root 'build/game-build-layout'
New-Item -ItemType Directory -Force $out | Out-Null
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vc = Join-Path $vs 'VC/Auxiliary/Build/vcvarsall.bat'
$source = Join-Path $root 'spikes/view_matrix/tests/test-game-build-layout.cpp'
Push-Location $out
try {
    cmd /c "`"$vc`" x86 >nul && cl /nologo /O2 /EHsc /MD /std:c++17 `"$source`" /Fe:test-game-build-layout.exe"
    if ($LASTEXITCODE -ne 0) { throw 'Layout fixture compilation failed' }
    # Only our fixture is executed. EXE arguments are parsed as PE data.
    & ./test-game-build-layout.exe $english english-map
    if ($LASTEXITCODE -ne 0) { throw 'English real-PE contract fixture failed' }
    & ./test-game-build-layout.exe $german alternate-map
    if ($LASTEXITCODE -ne 0) { throw 'German real-PE contract fixture failed' }
    if ($retail) {
        & ./test-game-build-layout.exe $retail unsupported
        if ($LASTEXITCODE -ne 0) { throw 'Protected retail refusal fixture failed' }
    }
    $retain = Join-Path $root 'spikes/view_matrix/tests/test-locker-retain-policy.cpp'
    cmd /c "`"$vc`" x86 >nul && cl /nologo /O2 /EHsc /MD /std:c++17 `"$retain`" /Fe:test-locker-retain-policy.exe"
    if ($LASTEXITCODE -ne 0) { throw 'Locker retention fixture compilation failed' }
    & ./test-locker-retain-policy.exe
    if ($LASTEXITCODE -ne 0) { throw 'English locker retention policy failed' }
    & ./test-locker-retain-policy.exe german
    if ($LASTEXITCODE -ne 0) { throw 'German locker retention policy failed' }
} finally { Pop-Location }
