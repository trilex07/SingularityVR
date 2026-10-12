# Builds the standalone texture-override injector as a 32-bit dinput8.dll.
#
# Deploy naming is defined in ONE place on each side of the boundary and is
# asserted to match:
#   runtime lookup  : asset-tools/texture-injector/texq_naming.h  (QualitySuffix)
#   deploy naming   : $deploySuffix below
# tier 2x -> "<name>_2x.dds", 4K -> "<name>_4K.dds", 8K -> "<name>_8K.dds"
#
# The DLL co-exists with the VR mod's d3d9.dll (the game statically imports
# DINPUT8!DirectInput8Create and no dinput8.dll ships in Binaries). Reversible:
# delete the deployed items to restore vanilla. Nothing here writes to Steam
# unless -Install is given AND -GameBin points at it.
param(
    [switch]$Install,
    [ValidateSet("A", "B")]
    [string]$Variant = "A",
    [ValidateSet("2x", "4K", "8K")]
    [string]$Quality = "2x",
    [string]$GameBin = "C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries",
    [string]$LocalRoot = "C:\Users\Trilex\vr-mods\SingularityVR-assets-local",
    # extra directory of ready-made "<name>_<tier>.dds" files (the material
    # remaster output) that is deployed alongside the pipeline candidates
    [string]$RemasterDir = "",
    [switch]$NoBuild
)

$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Resolve-Path (Join-Path $here "..\..")
$mh   = Join-Path $root "third_party\minhook"
if (-not (Test-Path $mh)) { throw "MinHook not found: $mh" }

$deploySuffix = switch ($Quality) {
    "4K" { "_4K" }
    "8K" { "_8K" }
    default { "_2x" }
}
Write-Host "Quality tier: $Quality -> deploy suffix $deploySuffix"

Push-Location $here
try {
    if (-not $NoBuild) {
        $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
        $vsPath  = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        $vcvars  = Join-Path $vsPath "VC\Auxiliary\Build\vcvarsall.bat"

        $mhSrc = @(
            "`"$mh\src\buffer.c`"",
            "`"$mh\src\hook.c`"",
            "`"$mh\src\trampoline.c`"",
            "`"$mh\src\hde\hde32.c`""
        ) -join " "

        $cmd = "`"$vcvars`" x86 && cl /nologo /LD /EHsc /W3 /O2 /MD /std:c++17 " +
               "/I`"$mh\include`" " +
               "/Fe:dinput8.dll texture_injector.cpp $mhSrc " +
               "/link /DEF:dinput8.def user32.lib"
        cmd /c $cmd
        if ($LASTEXITCODE -ne 0) { throw "build failed (exit $LASTEXITCODE)" }
        Write-Host "Built: $here\dinput8.dll"
    }

    if ($Install) {
        if (-not (Test-Path $GameBin)) { throw "game Binaries not found: $GameBin" }

        $texDst = Join-Path $GameBin "texture-override"
        New-Item -ItemType Directory -Force -Path $texDst | Out-Null
        Get-ChildItem $texDst -Filter *.dds -ErrorAction SilentlyContinue | Remove-Item -Force

        # ---- 1. pipeline candidates (variant A/B Upscaled output) ----------
        $upDir = if ($Quality -eq "2x") { "textures\upscaled" } else { "textures\upscaled-$Quality" }
        $up = Join-Path $LocalRoot $upDir
        $gen = Join-Path $here "targets_generated.h"
        if ((Test-Path $up) -and (Test-Path $gen)) {
            $suffix = if ($Variant -eq "B") { "__B_ibp" } else { "__A_lanczos" }
            $wanted = Select-String -Path $gen -Pattern '"([^"]+)\.dds"' -AllMatches |
                ForEach-Object { $_.Matches } | ForEach-Object { $_.Groups[1].Value } |
                Sort-Object -Unique
            $strip = "$suffix.dds"
            $count = 0; $missing = @()
            foreach ($base in $wanted) {
                $from = Join-Path $up "$base$strip"
                if (Test-Path $from) {
                    Copy-Item $from (Join-Path $texDst "$base$deploySuffix.dds") -Force
                    $count++
                } else {
                    $missing += "$base$strip"
                }
            }
            Write-Host "Variant: $Variant ($suffix) Quality: $Quality, $count of $($wanted.Count) target(s) deployed as *$deploySuffix.dds"
            if ($missing.Count -gt 0) { Write-Warning ("missing candidates: " + ($missing -join ", ")) }
        } else {
            Write-Warning "pipeline output not present ($up) - skipping candidate deploy"
        }

        # ---- 2. material remaster package ---------------------------------
        if ($RemasterDir) {
            if (-not (Test-Path $RemasterDir)) { throw "RemasterDir not found: $RemasterDir" }
            $remasterFiles = Get-ChildItem -Path $RemasterDir -Recurse -Filter "*$deploySuffix.dds"
            $n = 0
            foreach ($f in $remasterFiles) {
                if ($f.Name -like "*_specular_review*") {
                    Write-Host "  skipping review-only file $($f.Name)"
                    continue
                }
                Copy-Item $f.FullName (Join-Path $texDst $f.Name) -Force
                $n++
            }
            Write-Host "Remaster: $n file(s) deployed from $RemasterDir"
        }

        # ---- 3. proxy plumbing -------------------------------------------
        $sysDi8 = Join-Path ${env:windir} "SysWOW64\dinput8.dll"
        if (-not (Test-Path $sysDi8)) { throw "system dinput8 not found: $sysDi8" }
        Copy-Item $sysDi8 (Join-Path $GameBin "dinput8_orig.dll") -Force

        # with -NoBuild the DLL may legitimately not exist yet (texture-only
        # deploy / naming test); do not fail the deploy for that
        $localDll = Join-Path $here "dinput8.dll"
        if (Test-Path $localDll) {
            Copy-Item $localDll $GameBin -Force
        } elseif (-not $NoBuild) {
            throw "dinput8.dll missing after build: $localDll"
        } else {
            Write-Warning "dinput8.dll not present (-NoBuild): DLL not copied"
        }
        Copy-Item (Join-Path $here "texture_override.ini") $GameBin -Force

        Write-Host "Installed to: $GameBin"
        Write-Host "  dinput8.dll + dinput8_orig.dll + texture_override.ini + texture-override\*$deploySuffix.dds"
        Write-Host "Restore vanilla: delete dinput8.dll, dinput8_orig.dll, texture_override.ini and texture-override\ from Binaries."
    }
} finally {
    Pop-Location
}
