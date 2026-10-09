# Builds the standalone texture-override injector as a 32-bit dinput8.dll.
# -Install copies it + the hand replacement textures into the game's Binaries,
# where it co-exists with the VR mod's d3d9.dll. Reversible: delete the three
# deployed items to restore vanilla. See README.md.
param(
    [switch]$Install,
    [ValidateSet("A", "B")]
    [string]$Variant = "A",
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
        # clear old variant files so only the chosen one is present
        Get-ChildItem $texDst -Filter *.dds -ErrorAction SilentlyContinue | Remove-Item -Force

        # Deploy ONLY the filenames listed in targets_generated.h (the injector's
        # lookup table). Stale candidates from earlier runs must never ship:
        # a file without a target either matches nothing or, worse, matches a
        # texture whose source was only a lower mip.
        $suffix = if ($Variant -eq "B") { "__B_ibp" } else { "__A_lanczos" }
        $up = Join-Path $LocalRoot "textures\upscaled"
        if (-not (Test-Path $up)) { throw "upscaled dir missing: $up (run the pipeline 'upscale' step first)" }
        $gen = Join-Path $here "targets_generated.h"
        if (-not (Test-Path $gen)) { throw "targets_generated.h missing (run the pipeline 'targets' step first)" }
        $wanted = Select-String -Path $gen -Pattern '"([^"]+)\.dds"' -AllMatches |
            ForEach-Object { $_.Matches } | ForEach-Object { $_.Groups[1].Value } |
            Sort-Object -Unique
        $strip = "$suffix.dds"
        $count = 0; $missing = @()
        foreach ($base in $wanted) {
            $from = Join-Path $up "$base$strip"
            if (Test-Path $from) {
                Copy-Item $from (Join-Path $texDst "$base.dds") -Force
                $count++
            } else {
                $missing += "$base$strip"
            }
        }
        if ($count -eq 0) { throw "no '$suffix' candidates found in $up" }
        if ($missing.Count -gt 0) { Write-Warning ("missing candidates: " + ($missing -join ", ")) }
        Write-Host "Variant: $Variant ($suffix), $count of $($wanted.Count) target(s) deployed"

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
