param(
    [string]$LogPath = "$env:LOCALAPPDATA\SingularityVR\view_matrix.log",
    [string]$Validator = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path $PSScriptRoot -Parent
if (-not $Validator) {
    $Validator = Join-Path $repoRoot "spikes\view_matrix\tests\test-pause-compose-trace.exe"
}
if (-not (Test-Path -LiteralPath $LogPath)) {
    Write-Error "Log not found: $LogPath"
}
if (-not (Test-Path -LiteralPath $Validator)) {
    Write-Error "Build the validator first: spikes/view_matrix/tests/test-pause-compose-trace.cpp"
}
& $Validator $LogPath
exit $LASTEXITCODE
