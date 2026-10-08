# Builds the standalone texture-override injector as a 32-bit dinput8.dll.
# -Install copies it + the hand replacement textures into the game's Binaries,
# where it co-exists with the VR mod's d3d9.dll. Reversible: delete the three
# deployed items to restore vanilla. See README.md.
param(
    [switch]$Install,
    [string]$GameBin = "C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries",
    [string]$LocalRoot = "C:\Users\Trilex\vr-mods\SingularityVR-assets-local"
)

$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Resolve-Path (Join-Path $here "..\..")
$mh   = Join-Path $root "third_party\minhook"
if (-not (Test-Path $mh)) { throw "MinHook not found: $mh" }

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath  = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars  = Join-Path $vsPath "VC\Auxiliary\Build\vcvarsall.bat"

Push-Location $here
try {
    $mhSrc = @(
        "`"$mh\src\buffer.c`"",
        "`"$mh\src\hook.c`"",
        "`"$mh\src\trampoline.c`"",
        "`"$mh\src\hde\hde32.c`""
    ) -join " "

    $cmd = "`"$vcvars`" x86 && cl /nologo /LD /EHsc /W3 /MD /std:c++17 " +
           "/I`"$mh\include`" " +
           "/Fe:dinput8.dll texture_injector.cpp $mhSrc " +
           "/link /DEF:dinput8.def user32.lib"
    cmd /c $cmd
    if ($LASTEXITCODE -ne 0) { throw "build failed (exit $LASTEXITCODE)" }
    Write-Host "Built: $here\dinput8.dll"

    if ($Install) {
        if (-not (Test-Path $GameBin)) { throw "game Binaries not found: $GameBin" }

        $texDst = Join-Path $GameBin "texture-override"
        New-Item -ItemType Directory -Force -Path $texDst | Out-Null

        # Map the chosen candidate -> injector filename baked into the DLL.
        $map = @{
            "inview_glove_d__A_lanczos.dds" = "inview_glove_d.A.dds"
            "inview_glove_n__A_lanczos.dds" = "inview_glove_n.A.dds"
        }
        $up = Join-Path $LocalRoot "textures\upscaled"
        foreach ($src in $map.Keys) {
            $from = Join-Path $up $src
            if (-not (Test-Path $from)) { throw "candidate missing: $from (run the pipeline 'upscale' step first)" }
            Copy-Item $from (Join-Path $texDst $map[$src]) -Force
        }

        # forward to a local copy of the real dinput8 under a distinct name
        $sysDi8 = Join-Path ${env:windir} "SysWOW64\dinput8.dll"
        if (-not (Test-Path $sysDi8)) { throw "system dinput8 not found: $sysDi8" }
        Copy-Item $sysDi8 (Join-Path $GameBin "dinput8_orig.dll") -Force

        Copy-Item (Join-Path $here "dinput8.dll") $GameBin -Force
        Copy-Item (Join-Path $here "texture_override.ini") $GameBin -Force
        Write-Host "Installed to: $GameBin"
        Write-Host "  dinput8.dll + dinput8_orig.dll + texture_override.ini + texture-override\*.dds"
        Write-Host "Restore vanilla: delete dinput8.dll, dinput8_orig.dll, texture_override.ini and texture-override\ from Binaries."
    }
} finally {
    Pop-Location
}
