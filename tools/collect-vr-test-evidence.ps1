[CmdletBinding()]
param(
    [string]$DestinationRoot = '',
    [int]$NewestCount = 1
)

# Collects evidence only; it does not launch or modify the game.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$sourceRoot = Join-Path $env:LOCALAPPDATA 'SingularityVR'
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$destination = if ($DestinationRoot) { $DestinationRoot } else {
    Join-Path $repoRoot "deployment\vr-user-test-$stamp"
}
New-Item -ItemType Directory -Force -Path $destination | Out-Null

$files = @()
foreach ($pattern in @('capture_*.bmp', 'view_matrix*.log')) {
    $files += @(Get-ChildItem -LiteralPath $sourceRoot -Filter $pattern -File -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -First $NewestCount)
}
$sessionDir = Join-Path $sourceRoot 'vr_sessions'
if (Test-Path -LiteralPath $sessionDir -PathType Container) {
    $files += @(Get-ChildItem -LiteralPath $sessionDir -Filter '*.vrrec' -File -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -First $NewestCount)
}

$entries = foreach ($file in $files | Sort-Object FullName -Unique) {
    $target = Join-Path $destination $file.Name
    Copy-Item -LiteralPath $file.FullName -Destination $target -Force
    [ordered]@{
        source = $file.FullName
        copied = $target
        bytes = $file.Length
        sha256 = (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash
        last_write = $file.LastWriteTime.ToString('o')
    }
}

$manifest = [ordered]@{
    collected = (Get-Date).ToString('o')
    source_root = $sourceRoot
    destination = $destination
    files = @($entries)
    next_step = 'Run tools/vr_session_dump.py and tools/vr_session_replay.py on the copied .vrrec.'
}
$manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $destination 'manifest.json') -Encoding UTF8
$manifest | ConvertTo-Json -Depth 6
Write-Host "Evidence directory: $destination"
