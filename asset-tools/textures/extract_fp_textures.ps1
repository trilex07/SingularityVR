# Thin wrapper around the Python pipeline 'build' stage.
# Extracts first-person TMD hand/arm and weapon textures to the LOCAL scratch
# root (never into the repo, never into the game). See README.md.
[CmdletBinding()]
param(
    [string]$Game = "C:\Program Files (x86)\Steam\steamapps\common\Singularity",
    [string]$LocalRoot = "C:\Users\Trilex\vr-mods\SingularityVR-assets-local",
    [string]$UModel = "C:\Users\Trilex\vr-mods\research\tools\UEViewer\umodel.exe"
)
$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
python (Join-Path $here "texture_pipeline.py") --game $Game --local-root $LocalRoot --umodel $UModel build
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Host "Extraction complete. Pixels are under: $LocalRoot\textures"
