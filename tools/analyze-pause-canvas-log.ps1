param(
    [string]$LogPath = "$env:LOCALAPPDATA\SingularityVR\view_matrix.log",
    [string]$Validator = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
if (-not $Validator) {
    $Validator = Join-Path $repoRoot "spikes\view_matrix\tests\test-pause-canvas-trace.exe"
}
if (-not (Test-Path -LiteralPath $LogPath)) { throw "Log not found: $LogPath" }
if (-not (Test-Path -LiteralPath $Validator)) { throw "Build test-pause-canvas-trace.cpp first" }
$lines = (Select-String -Path $LogPath -Pattern "pause-canvas txn=").Count
Write-Host "pause-canvas draw/event lines in log: $lines"
& $Validator $LogPath
exit $LASTEXITCODE
