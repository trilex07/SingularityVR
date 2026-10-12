# Removes the standalone texture-override injector from the game and restores
# vanilla. Only the four items this tool installed are deleted; nothing else is
# touched.
param(
    [string]$GameBin = "C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries"
)
$ErrorActionPreference = "Stop"
if (-not (Test-Path $GameBin)) { throw "game Binaries not found: $GameBin" }

foreach ($f in @("dinput8.dll", "dinput8_orig.dll", "texture_override.ini", "texture_override.log")) {
    $p = Join-Path $GameBin $f
    if (Test-Path $p) { Remove-Item $p -Force; Write-Host "removed $f" }
}
$tex = Join-Path $GameBin "texture-override"
if (Test-Path $tex) { Remove-Item $tex -Recurse -Force; Write-Host "removed texture-override\" }
Write-Host "Vanilla restored."
