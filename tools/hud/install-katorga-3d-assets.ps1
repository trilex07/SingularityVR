param(
    [Parameter(Mandatory = $true)]
    [string]$GameRoot
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$src3d = Join-Path $repo 'assets\hud\katorga\3d'
$srcAtlas = Join-Path $repo 'assets\hud\katorga\atlas'
$dst3d = Join-Path $GameRoot 'vr-assets\hud\katorga\3d'
$dstAtlas = Join-Path $GameRoot 'vr-assets\hud\katorga\atlas'

if (-not (Test-Path $src3d)) { throw "Missing SIDE pack: $src3d" }

function Sync-Tree($from, $to) {
    New-Item -ItemType Directory -Force -Path $to | Out-Null
    robocopy $from $to /E /NFL /NDL /NJH /NJS /nc /ns /np | Out-Null
    if ($LASTEXITCODE -ge 8) { throw "robocopy failed $from -> $to exit=$LASTEXITCODE" }
}

Sync-Tree $src3d $dst3d
if (Test-Path $srcAtlas) { Sync-Tree $srcAtlas $dstAtlas }

Write-Host "Installed Katorga HUD assets to $dst3d"
Write-Host "Atlas (if present): $dstAtlas"
