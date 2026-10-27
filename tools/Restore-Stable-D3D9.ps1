# Restore a known-good mod d3d9.dll (Runtime Error recovery).
# Default = manual backup only. Newer spikes builds need Restore-Working-Launch.ps1 + dinput8 check.
[CmdletBinding()]
param(
    [string]$SteamRoot = 'C:\Program Files (x86)\Steam',
    [switch]$DisableTextureInjector,
    [switch]$UseLatestBuild
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$built = Join-Path $repo 'spikes\view_matrix\d3d9.dll'
$manual = Join-Path $repo 'deployment\install-backups\manual-20261009-context\d3d9.dll'
$bin = Join-Path $SteamRoot 'steamapps\common\Singularity\Binaries'

$source = $null
if ($UseLatestBuild -and (Test-Path -LiteralPath $built)) {
    $source = $built
} elseif (Test-Path -LiteralPath $manual) {
    $source = $manual
} elseif (Test-Path -LiteralPath $built) {
    $source = $built
} else {
    throw "No stable d3d9.dll found (build or manual backup)."
}

Get-Process -Name Singularity -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 2
Copy-Item -LiteralPath $source -Destination (Join-Path $bin 'd3d9.dll') -Force
if ($DisableTextureInjector) {
    $d = Join-Path $bin 'dinput8.dll'
    if (Test-Path -LiteralPath $d) {
        Rename-Item -LiteralPath $d -NewName 'dinput8.dll.disabled' -Force
        Write-Host 'dinput8.dll -> dinput8.dll.disabled'
    }
}
$sha = (Get-FileHash (Join-Path $bin 'd3d9.dll') -Algorithm SHA256).Hash
Write-Host "Installed from: $source"
Write-Host "SHA-256: $sha"
Write-Host 'Launch: tools\Start-SingularityVR-Headset.ps1'
