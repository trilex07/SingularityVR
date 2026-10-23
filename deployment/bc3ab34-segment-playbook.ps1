# Archive SingularityVR segment logs for bc3ab34 A/B/C PSVR2 validation.
param(
    [Parameter(Mandatory=$true)][ValidateSet('A','B','C')][string]$Segment,
    [string]$Note = ''
)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$log = Join-Path $env:LOCALAPPDATA 'SingularityVR\view_matrix.log'
$destRoot = Join-Path $repo "deployment/crash-evidence/20261023-bc3ab34-segment-$($Segment.ToLower())"
New-Item -ItemType Directory -Force -Path $destRoot | Out-Null
if (-not (Test-Path -LiteralPath $log)) { throw "Missing log: $log" }
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$archived = Join-Path $destRoot "view_matrix.log"
Copy-Item -LiteralPath $log -Destination $archived -Force
$hash = (Get-FileHash -LiteralPath $archived -Algorithm SHA256).Hash
$meta = [ordered]@{
    segment = $Segment
    archivedAt = (Get-Date).ToString('o')
    sha256 = $hash
    bytes = (Get-Item -LiteralPath $archived).Length
    note = $Note
}
($meta | ConvertTo-Json) | Set-Content (Join-Path $destRoot 'manifest.json')
Write-Host "Archived segment $Segment -> $archived"
Write-Host "SHA256 $hash"
# Fresh log on next game start (mod renames if configured) or delete for clean boundary:
Remove-Item -LiteralPath $log -Force -ErrorAction SilentlyContinue
