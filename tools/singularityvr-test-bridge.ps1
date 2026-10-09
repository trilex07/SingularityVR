[CmdletBinding()]
param(
    [ValidateSet('Preflight', 'Collect', 'VerifyCapture')]
    [string]$Action = 'Preflight',
    [string]$Manifest = '',
    [string]$Baseline = '',
    [string]$Previous = '',
    [string[]]$Roi = @(),
    [string[]]$EffectRoi = @(),
    [int]$ExpectedPid = 0,
    [string]$ExpectedDllSha256 = '',
    [string]$Report = '',
    [string]$XrSimDirectory = '',
    [string]$CaptureJson = '',
    [string]$AckPath = '',
    [string]$Scenario = '',
    [string]$DllPath = '',
    [string]$Destination = ''
)

# Read-only Test Bridge operations. VerifyCapture never installs, loads, or
# replaces a DLL. Deployment remains owned by the main agent's runner.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repoRoot = Split-Path -Parent $PSScriptRoot
$oracle = Join-Path $repoRoot 'tools\vr_capture_oracle.py'

function Get-PeIdentity([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return $null }
    $bytes = [IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 0x40 -or $bytes[0] -ne 0x4d -or $bytes[1] -ne 0x5a) { return $null }
    $offset = [BitConverter]::ToInt32($bytes, 0x3c)
    if ($offset -lt 0 -or $offset + 0x1a -gt $bytes.Length) { return $null }
    if ([BitConverter]::ToUInt32($bytes, $offset) -ne 0x4550) { return $null }
    $machine = [BitConverter]::ToUInt16($bytes, $offset + 4)
    $magic = [BitConverter]::ToUInt16($bytes, $offset + 24)
    return [ordered]@{ machine = ('0x{0:X4}' -f $machine); optionalHeaderMagic = ('0x{0:X4}' -f $magic); x86Pe32 = ($machine -eq 0x14c -and $magic -eq 0x10b) }
}

if ($Action -eq 'Preflight') {
    $dll = Join-Path $repoRoot 'spikes\view_matrix\d3d9.dll'
    $identity = Get-PeIdentity $dll
    $hash = if (Test-Path -LiteralPath $dll) { (Get-FileHash -LiteralPath $dll -Algorithm SHA256).Hash.ToUpperInvariant() } else { $null }
    $result = [ordered]@{ action = 'Preflight'; repository = $repoRoot; dll = $dll; sha256 = $hash; pe = $identity; readOnly = $true; pass = [bool]($identity -and $identity.x86Pe32 -and $hash) }
    $result | ConvertTo-Json -Depth 5
    if (-not $result.pass) { exit 2 }
    exit 0
}

if ($Action -eq 'Collect') {
    if (-not $XrSimDirectory -or -not $CaptureJson -or -not $AckPath -or -not $Scenario -or -not $DllPath -or -not $Destination) {
        throw 'Collect requires XrSimDirectory, CaptureJson, AckPath, Scenario, DllPath, and Destination.'
    }
    $statePath = Join-Path $XrSimDirectory 'state.json'
    foreach ($path in @($statePath, $CaptureJson, $AckPath, $DllPath)) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Collect input missing: $path" }
    }
    $state = Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
    $capture = Get-Content -LiteralPath $CaptureJson -Raw | ConvertFrom-Json
    $ack = Get-Content -LiteralPath $AckPath -Raw
    if ($ack -notmatch 'seq=(\d+)\s+frame=(\d+)\s+error=([^\s]+)') { throw 'ack.txt has no parseable seq/frame/error contract.' }
    $cmdSeq = [int]$Matches[1]
    $ackFrame = [int]$Matches[2]
    $ackError = [string]$Matches[3]
    if ($ackError -ne '-') { throw "ack reports an error: $ackError" }
    if ([int]$capture.frameIndex -ne ($ackFrame + 1)) { throw "capture frame $($capture.frameIndex) is not the frame after acknowledged command $ackFrame" }
    if ([int]$state.eyeReleasedOnFrameL -ne [int]$capture.frameIndex -or [int]$state.eyeReleasedOnFrameR -ne [int]$capture.frameIndex) {
        throw "state eye release frames do not match capture frame $($capture.frameIndex); capture is stale or mixed."
    }
    $captureDir = Split-Path -Parent $CaptureJson
    $stem = [IO.Path]::GetFileNameWithoutExtension($CaptureJson)
    $left = Join-Path $captureDir ($stem + '_left.png')
    $right = Join-Path $captureDir ($stem + '_right.png')
    foreach ($path in @($left, $right)) { if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Eye image missing: $path" } }
    $dllSha = (Get-FileHash -LiteralPath $DllPath -Algorithm SHA256).Hash.ToUpperInvariant()
    $source = if (@($capture.layers | Where-Object { $_.type -eq 'projection' }).Count) { 'xr_projection_layer' } else { 'other' }
    $manifest = [ordered]@{
        schema = 'singularityvr.capture.v1'
        runId = "xrsim-$($state.pid)-$($capture.frameIndex)-$($state.captureSeq)"
        scenario = $Scenario
        cmdSeq = $cmdSeq
        frameIndex = [int]$capture.frameIndex
        captureSeq = [int]$state.captureSeq
        pid = [int]$state.pid
        dllSha256 = $dllSha
        source = $source
        stereoOrderKnown = $true
        acknowledgement = [ordered]@{ seq = $cmdSeq; frame = $ackFrame; error = $ackError; raw = $ack.Trim() }
        eyes = @(
            [ordered]@{ eye = 'L'; path = [IO.Path]::GetFullPath($left); sha256 = (Get-FileHash -LiteralPath $left -Algorithm SHA256).Hash.ToUpperInvariant(); cmdSeq = $cmdSeq; frameIndex = [int]$capture.frameIndex; captureSeq = [int]$state.captureSeq; source = $source },
            [ordered]@{ eye = 'R'; path = [IO.Path]::GetFullPath($right); sha256 = (Get-FileHash -LiteralPath $right -Algorithm SHA256).Hash.ToUpperInvariant(); cmdSeq = $cmdSeq; frameIndex = [int]$capture.frameIndex; captureSeq = [int]$state.captureSeq; source = $source }
        )
    }
    New-Item -ItemType Directory -Force -Path $Destination | Out-Null
    $manifestPath = Join-Path $Destination ($stem + '.capture.json')
    $manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $manifestPath -Encoding UTF8
    [ordered]@{ action = 'Collect'; manifest = $manifestPath; pass = $true } | ConvertTo-Json -Depth 8
    exit 0
}

if (-not (Test-Path -LiteralPath $oracle -PathType Leaf)) { throw "Oracle missing: $oracle" }
if (-not (Test-Path -LiteralPath $Manifest -PathType Leaf)) { throw "Capture manifest missing: $Manifest" }
$arguments = @($oracle, $Manifest)
if ($Baseline) { $arguments += @('--baseline', $Baseline) }
if ($Previous) { $arguments += @('--previous', $Previous) }
foreach ($item in $Roi) { $arguments += @('--roi', $item) }
foreach ($item in $EffectRoi) { $arguments += @('--effect-roi', $item) }
if ($ExpectedPid -gt 0) { $arguments += @('--expected-pid', $ExpectedPid) }
if ($ExpectedDllSha256) { $arguments += @('--expected-dll', $ExpectedDllSha256) }
if ($Report) { $arguments += @('--report', $Report) }
& py -3 @arguments
exit $LASTEXITCODE
