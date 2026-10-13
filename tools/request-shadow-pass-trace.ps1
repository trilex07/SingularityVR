[CmdletBinding()]
param([ValidateSet('ON','OFF','CONTROL')][string]$Label='CONTROL')
$ErrorActionPreference='Stop'
# Diagnostic request only; does NOT modify the game's HQ decals option or INI.
$traceRoot=Join-Path $env:USERPROFILE 'AppData\Local\SingularityVR'
if (-not (Test-Path -LiteralPath $traceRoot -PathType Container)) { throw 'No runtime log directory' }
$request=Join-Path $traceRoot 'shadow-pass.request'
if (Test-Path -LiteralPath $request) { throw 'A request is already pending' }
[ordered]@{label=$Label;requestedAt=(Get-Date).ToString('o')} |
    ConvertTo-Json | Set-Content -LiteralPath $request -Encoding utf8
Write-Output "Requested one read-only gameplay frame, label=$Label; set HQ decals separately in game"
