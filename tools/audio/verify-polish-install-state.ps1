[CmdletBinding()]
param(
    [string]$GameRoot = 'C:\Program Files (x86)\Steam\steamapps\common\Singularity',
    [string]$RepoRoot = ''
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (-not $RepoRoot) {
    $here = Split-Path -Parent $MyInvocation.MyCommand.Path
    $RepoRoot = (Resolve-Path (Join-Path $here '../..')).Path
}

function Get-Sha256([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return 'ABSENT' }
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
}

$core = Join-Path $RepoRoot 'deployment/hands-dubbing/recovery-ready/core/test-manifest.json'
$assets = Join-Path $RepoRoot 'deployment/hands-dubbing/recovery-ready/assets/asset-manifest.json'
$storyReceipt = Join-Path $RepoRoot 'deployment/hands-dubbing/story-xlingual-installed-20261014/story-install.json'

foreach ($path in @($core, $assets, $storyReceipt)) {
    if (-not (Test-Path -LiteralPath $path)) { throw "Missing manifest: $path" }
}

$coreManifest = Get-Content -LiteralPath $core -Raw | ConvertFrom-Json
$assetManifest = Get-Content -LiteralPath $assets -Raw | ConvertFrom-Json
$storyManifest = Get-Content -LiteralPath $storyReceipt -Raw | ConvertFrom-Json

$rows = [System.Collections.Generic.List[object]]::new()
function Add-Row([string]$Kind, [string]$RelativePath, [string]$Expected, [string]$Actual) {
    $rows.Add([pscustomobject]@{
            Kind         = $Kind
            RelativePath = $RelativePath
            Expected     = $Expected
            Actual       = $Actual
            Match        = ($Expected -ceq $Actual)
        }) | Out-Null
}

foreach ($f in $coreManifest.Files) {
    $target = Join-Path $GameRoot $f.RelativePath
    Add-Row 'core' $f.RelativePath $f.CandidateSha256 (Get-Sha256 $target)
}

foreach ($f in $assetManifest.Files) {
    $target = Join-Path $GameRoot $f.RelativePath
    Add-Row 'asset' $f.RelativePath $f.CandidateSha256 (Get-Sha256 $target)
}

$storyDir = Join-Path $GameRoot 'Binaries\vr-assets\audio\polish\story'
$wavCount = if (Test-Path -LiteralPath $storyDir) { @(Get-ChildItem -LiteralPath $storyDir -Filter '*.wav').Count } else { 0 }
$storyExpected = @($storyManifest.Files).Count
Add-Row 'story-wav-count' 'Binaries\vr-assets\audio\polish\story\*.wav' "$storyExpected" "$wavCount"

$mismatch = @($rows | Where-Object { -not $_.Match })
$summary = [ordered]@{
    CreatedAt       = (Get-Date).ToString('o')
    GameRoot        = $GameRoot
    RepoRoot        = $RepoRoot
    CoreMatch       = (@($rows | Where-Object { $_.Kind -eq 'core' -and $_.Match }).Count)
    CoreTotal       = (@($rows | Where-Object { $_.Kind -eq 'core' }).Count)
    AssetMatch      = (@($rows | Where-Object { $_.Kind -eq 'asset' -and $_.Match }).Count)
    AssetTotal      = (@($rows | Where-Object { $_.Kind -eq 'asset' }).Count)
    StoryWavMatch   = ($wavCount -eq $storyExpected)
    AllMatch        = ($mismatch.Count -eq 0)
    MismatchCount   = $mismatch.Count
}

$outDir = Join-Path $RepoRoot 'deployment/hands-dubbing/current-install-freeze-20261014'
New-Item -ItemType Directory -Force -Path $outDir | Out-Null
$report = Join-Path $outDir 'steam-verify-latest.json'
@{
    Summary = $summary
    Rows    = $rows
} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $report -Encoding UTF8

Write-Output ("verify report: " + $report)
Write-Output ("core " + $summary.CoreMatch + '/' + $summary.CoreTotal + ' asset ' + $summary.AssetMatch + '/' + $summary.AssetTotal + ' storyWav ' + $wavCount + '/' + $storyExpected + ' all=' + $summary.AllMatch)
if ($mismatch.Count -gt 0) {
    $mismatch | Format-Table -AutoSize
    exit 2
}
