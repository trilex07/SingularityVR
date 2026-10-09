[CmdletBinding()]
param(
    [ValidateSet('L1', 'L2', 'Package')]
    [string]$Action = 'L1',
    [string]$Recording = '',
    [string]$XrSimDirectory = '',
    [string]$Artifact = '',
    [string]$BuildManifest = '',
    [string]$EvidenceDirectory = '',
    [string]$HandoffPath = ''
)

# L1/L2 only. This script never launches Singularity, changes Steam, deploys a
# DLL, or starts OpenXR. It emits evidence for the main agent's deliberate L3.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = Split-Path -Parent $PSScriptRoot
if (-not $EvidenceDirectory) { $EvidenceDirectory = Join-Path $repo 'artifacts\agent-handoff' }
New-Item -ItemType Directory -Force -Path $EvidenceDirectory | Out-Null

function Invoke-Report([string]$Name, [scriptblock]$Command) {
    $output = & $Command 2>&1 | Out-String
    $code = $LASTEXITCODE
    $path = Join-Path $EvidenceDirectory $Name
    Set-Content -LiteralPath $path -Value $output -Encoding UTF8
    [ordered]@{ report = $path; exitCode = $code; output = $output }
}

if ($Action -eq 'L1') {
    $items = @()
    $items += Invoke-Report 'l1-oracle-tests.txt' { py -3 -m unittest discover -s (Join-Path $repo 'tools\tests') -p 'test_vr_capture_oracle.py' -v }
    if ($Recording) {
        if (-not (Test-Path -LiteralPath $Recording -PathType Leaf)) { throw "Recording missing: $Recording" }
        $dump = Invoke-Report 'l1-vrrec-dump.txt' { py -3 (Join-Path $repo 'tools\vr_session_dump.py') $Recording }
        $items += $dump
        if ($XrSimDirectory) {
            $dry = Join-Path $EvidenceDirectory 'l1-replay-dryrun.json'
            $items += Invoke-Report 'l1-replay-dryrun.txt' { py -3 (Join-Path $repo 'tools\xrsim_replay_vr_session.py') $Recording --dir $XrSimDirectory --stride 4 --dry-run --report $dry }
        }
    }
    $pass = [bool](@($items | Where-Object { $_.exitCode -ne 0 }).Count -eq 0)
    [ordered]@{ schema = 'singularityvr.agent-l1.v1'; action = 'L1'; createdAt = (Get-Date).ToString('o'); pass = $pass; evidence = $items } | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $EvidenceDirectory 'l1-report.json') -Encoding UTF8
    Get-Content -LiteralPath (Join-Path $EvidenceDirectory 'l1-report.json') -Raw
    if (-not $pass) { exit 2 }; exit 0
}

function Get-Pe([string]$Path) {
    $bytes = [IO.File]::ReadAllBytes($Path); if ($bytes.Length -lt 0x40 -or $bytes[0] -ne 0x4d -or $bytes[1] -ne 0x5a) { throw 'Not MZ' }; $off = [BitConverter]::ToInt32($bytes, 0x3c); if ([BitConverter]::ToUInt32($bytes, $off) -ne 0x4550) { throw 'Not PE' }; $machine = [BitConverter]::ToUInt16($bytes, $off + 4); $magic = [BitConverter]::ToUInt16($bytes, $off + 24); [ordered]@{ machine = ('0x{0:X4}' -f $machine); optionalHeaderMagic = ('0x{0:X4}' -f $magic); x86Pe32 = ($machine -eq 0x14c -and $magic -eq 0x10b) }
}
if ($Action -eq 'L2') {
    if (-not $Artifact) { throw 'L2 requires -Artifact.' }
    $sha = (Get-FileHash -LiteralPath $Artifact -Algorithm SHA256).Hash.ToUpperInvariant()
    $pe = Get-Pe $Artifact
    $manifest = if ($BuildManifest) { Get-Content -LiteralPath $BuildManifest -Raw | ConvertFrom-Json } else { $null }
    $checks = [ordered]@{ artifactExists = $true; x86Pe32 = [bool]$pe.x86Pe32; sha256 = $sha; manifestPresent = [bool]$manifest; manifestShaMatches = [bool]($manifest -and $manifest.sha256.ToUpperInvariant() -eq $sha); manifestO2 = [bool]($manifest -and $manifest.optimization -eq '/O2') }
    $pass = [bool]($checks.x86Pe32 -and $checks.manifestPresent -and $checks.manifestShaMatches -and $checks.manifestO2)
    $result = [ordered]@{ schema = 'singularityvr.agent-l2.v1'; action = 'L2'; createdAt = (Get-Date).ToString('o'); artifact = [IO.Path]::GetFullPath($Artifact); checks = $checks; pass = $pass }
    $result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $EvidenceDirectory 'l2-report.json') -Encoding UTF8
    Get-Content -LiteralPath (Join-Path $EvidenceDirectory 'l2-report.json') -Raw
    if (-not $pass) { exit 2 }; exit 0
}

if (-not $HandoffPath) { throw 'Package requires -HandoffPath.' }
$l1 = Join-Path $EvidenceDirectory 'l1-report.json'; $l2 = Join-Path $EvidenceDirectory 'l2-report.json'
if (-not (Test-Path $l1) -or -not (Test-Path $l2)) { throw 'Package requires completed L1 and L2 reports.' }
$commit = (& git -C $repo rev-parse HEAD).Trim()
[ordered]@{ schema = 'singularityvr.agent-handoff.v1'; createdAt = (Get-Date).ToString('o'); repository = $repo; commit = $commit; l1 = [IO.Path]::GetFullPath($l1); l2 = [IO.Path]::GetFullPath($l2); l3Owner = 'main-agent'; l3Deployment = 'explicit-main-agent-action-only'; steamTouched = $false } | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $HandoffPath -Encoding UTF8
Get-Content -LiteralPath $HandoffPath -Raw
