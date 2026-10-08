# Builds the reference VFX plugin stub as a 32-bit SingularityVfx.dll.
#   -Install copies SingularityVfx.dll + the atlas manifest into the game's
#   Binaries. Do NOT install until the plugin's desktop tests pass - a missing
#   DLL is a clean kill-switch, but dropping one in enables the effects.
param(
    [switch]$Install,
    [string]$GameBin = "C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries"
)
$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath  = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars  = Join-Path $vsPath "VC\Auxiliary\Build\vcvarsall.bat"

Push-Location $here
try {
    $cmd = "`"$vcvars`" x86 && cl /nologo /LD /EHsc /W4 /O2 /MD /std:c++17 " +
           "/Fe:SingularityVfx.dll vfx_plugin_stub.cpp"
    cmd /c $cmd
    if ($LASTEXITCODE -ne 0) { throw "build failed (exit $LASTEXITCODE)" }
    Write-Host "Built: $here\SingularityVfx.dll"

    # show the exports so the ABI names are visible/checked
    $out = cmd /c "`"$vcvars`" x86 >nul 2>nul && dumpbin /nologo /exports `"$here\SingularityVfx.dll`""
    $out | Select-String -Pattern "VfxPlugin" | ForEach-Object { $_.Line.Trim() }

    if ($Install) {
        if (-not (Test-Path $GameBin)) { throw "game Binaries not found: $GameBin" }
        Copy-Item (Join-Path $here "SingularityVfx.dll") $GameBin -Force
        Copy-Item (Join-Path $here "vfx_atlas_manifest.json") $GameBin -Force
        Write-Host "Installed SingularityVfx.dll + vfx_atlas_manifest.json to: $GameBin"
    }
} finally {
    Pop-Location
}
