[CmdletBinding()]
param(
    [ValidateSet('Preflight', 'Collect', 'Launch')]
    [string]$Action = 'Preflight',
    [string]$SteamRoot = 'C:\Program Files (x86)\Steam',
    [string]$DestinationRoot = '',
    [string]$LogRoot = '',
    [string]$XrSimDirectory = '',
    [int]$SteamVrWaitSeconds = 45,
    [int]$GameWaitSeconds = 30
)

# Bounded, local test orchestration. Preflight and Collect are read-only with
# respect to the game install; Launch delegates to the existing Steam test.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = Split-Path -Parent $PSScriptRoot
$repoParent = Split-Path -Parent $repoRoot
$builtDll = Join-Path $repoRoot 'spikes\view_matrix\d3d9.dll'
$buildManifestPath = Join-Path $repoRoot 'spikes\view_matrix\d3d9.build.json'
$steamBin = Join-Path $SteamRoot 'steamapps\common\Singularity\Binaries'
$deployedDll = Join-Path $steamBin 'd3d9.dll'
$logRoot = if ($LogRoot) { $LogRoot } else { Join-Path $env:LOCALAPPDATA 'SingularityVR' }
$xrSimDirectory = if ($XrSimDirectory) { $XrSimDirectory } elseif ($env:DVR_XRSIM_DIR) { $env:DVR_XRSIM_DIR } else { '' }

function Get-PeIdentity([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return $null }
    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 0x40 -or $bytes[0] -ne 0x4d -or $bytes[1] -ne 0x5a) { return $null }
    $offset = [BitConverter]::ToInt32($bytes, 0x3c)
    if ($offset -lt 0 -or $offset + 0x1a -gt $bytes.Length) { return $null }
    if ([BitConverter]::ToUInt32($bytes, $offset) -ne 0x00004550) { return $null }
    $machine = [BitConverter]::ToUInt16($bytes, $offset + 4)
    $magic = [BitConverter]::ToUInt16($bytes, $offset + 24)
    [ordered]@{
        machine = ('0x{0:X4}' -f $machine)
        optionalHeaderMagic = ('0x{0:X4}' -f $magic)
        x86Pe32 = ($machine -eq 0x014c -and $magic -eq 0x010b)
    }
}

function Get-FileHashUpper([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return $null }
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToUpperInvariant()
}

function Get-Preflight {
    $branch = (& git -C $repoRoot branch --show-current).Trim()
    $commit = (& git -C $repoRoot rev-parse HEAD).Trim()
    $status = @(& git -C $repoRoot status --short)
    $builtHash = Get-FileHashUpper $builtDll
    $deployedHash = Get-FileHashUpper $deployedDll
    $pe = Get-PeIdentity $builtDll
    $manifest = $null
    if (Test-Path -LiteralPath $buildManifestPath -PathType Leaf) {
        $manifest = Get-Content -LiteralPath $buildManifestPath -Raw | ConvertFrom-Json
    }
    $checks = [ordered]@{
        builtDllExists = [bool]$builtHash
        deployedDllExists = [bool]$deployedHash
        x86Pe32 = [bool]($pe -and $pe.x86Pe32)
        builtShaEqualsDeployedSha = [bool]($builtHash -and $deployedHash -and $builtHash -eq $deployedHash)
        manifestPresent = [bool]$manifest
        manifestSaysO2 = [bool]($manifest -and $manifest.optimization -eq '/O2')
        manifestHashMatchesBuilt = [bool]($manifest -and $manifest.sha256 -eq $builtHash)
        manifestCommitMatchesHead = [bool]($manifest -and $manifest.commit -eq $commit)
    }
    [ordered]@{
        action = 'Preflight'
        capturedAt = (Get-Date).ToString('o')
        repository = $repoRoot
        branch = $branch
        commit = $commit
        workingTreeStatus = $status
        builtDll = $builtDll
        builtSha256 = $builtHash
        deployedDll = $deployedDll
        deployedSha256 = $deployedHash
        pe = $pe
        buildManifest = $manifest
        checks = $checks
        pass = [bool](@($checks.Values | Where-Object { -not $_ }).Count -eq 0)
    }
}

function Write-Collection {
    $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
    $dest = if ($DestinationRoot) { $DestinationRoot } else {
        Join-Path $repoParent "deployment\agent-test-bridge\$stamp"
    }
    New-Item -ItemType Directory -Force -Path $dest | Out-Null

    $preflight = Get-Preflight
    $game = @(Get-Process -Name Singularity -ErrorAction SilentlyContinue | ForEach-Object {
        $started = $null; $path = $null
        try { $started = $_.StartTime.ToString('o') } catch { }
        try { $path = $_.Path } catch { }
        [ordered]@{ pid = $_.Id; started = $started; path = $path }
    })
    $vr = @('vrserver', 'vrcompositor', 'vrmonitor') | ForEach-Object {
        $name = $_
        Get-Process -Name $name -ErrorAction SilentlyContinue | ForEach-Object {
            [ordered]@{ name = $name; pid = $_.Id; started = $_.StartTime.ToString('o') }
        }
    }
    $copied = @()
    $latestLog = Join-Path $logRoot 'view_matrix.log'
    if (Test-Path -LiteralPath $latestLog -PathType Leaf) {
        $target = Join-Path $dest 'view_matrix.log'
        Copy-Item -LiteralPath $latestLog -Destination $target
        $copied += [ordered]@{ source = $latestLog; path = $target; sha256 = (Get-FileHashUpper $target) }
        $signalLines = @(Select-String -LiteralPath $latestLog -Pattern 'OpenXR|session running|ShadowMode|menu|prompt|VFX|frametime|frame time|fps|trigger|ERROR|failed' |
            Select-Object -Last 120 | ForEach-Object { $_.Line })
        $signalLines | Set-Content -LiteralPath (Join-Path $dest 'runtime-signals.txt') -Encoding UTF8
    }
    $capture = Get-ChildItem -LiteralPath $logRoot -Filter 'capture_*.bmp' -File -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -First 2
    foreach ($file in $capture) {
        $target = Join-Path $dest $file.Name
        Copy-Item -LiteralPath $file.FullName -Destination $target
        $copied += [ordered]@{ source = $file.FullName; path = $target; sha256 = (Get-FileHashUpper $target) }
    }
    $sessions = Join-Path $logRoot 'vr_sessions'
    if (Test-Path -LiteralPath $sessions -PathType Container) {
        $recording = Get-ChildItem -LiteralPath $sessions -Filter '*.vrrec' -File -ErrorAction SilentlyContinue |
            Sort-Object LastWriteTime -Descending | Select-Object -First 1
        if ($recording) {
            $target = Join-Path $dest $recording.Name
            Copy-Item -LiteralPath $recording.FullName -Destination $target
            $copied += [ordered]@{ source = $recording.FullName; path = $target; sha256 = (Get-FileHashUpper $target) }
        }
    }
    if ($xrSimDirectory -and (Test-Path -LiteralPath $xrSimDirectory -PathType Container)) {
        foreach ($name in @('state.json', 'xrsim.log')) {
            $source = Join-Path $xrSimDirectory $name
            if (Test-Path -LiteralPath $source -PathType Leaf) {
                $target = Join-Path $dest "xrsim-$name"
                Copy-Item -LiteralPath $source -Destination $target
                $copied += [ordered]@{ source = $source; path = $target; sha256 = (Get-FileHashUpper $target) }
            }
        }
    }
    $manifest = [ordered]@{
        action = 'Collect'
        collectedAt = (Get-Date).ToString('o')
        preflight = $preflight
        gameProcesses = $game
        steamVrProcesses = @($vr)
        files = $copied
        runtimeSignals = Join-Path $dest 'runtime-signals.txt'
        xrsimDirectory = if ($xrSimDirectory) { $xrSimDirectory } else { $null }
        limitations = 'Counters/log text are supporting evidence only; they do not prove correct stereo appearance or physical controller feel.'
    }
    $manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $dest 'manifest.json') -Encoding UTF8
    return [pscustomobject]@{ destination = $dest; manifest = $manifest }
}

switch ($Action) {
    'Preflight' {
        $result = Get-Preflight
        $result | ConvertTo-Json -Depth 8
        if (-not $result.pass) { throw 'Preflight failed; do not launch or test this DLL.' }
    }
    'Collect' {
        $result = Write-Collection
        $result | ConvertTo-Json -Depth 8
    }
    'Launch' {
        $preflight = Get-Preflight
        $preflight | ConvertTo-Json -Depth 8
        if (-not $preflight.pass) { throw 'Preflight failed; Steam launch was not attempted.' }
        $launcher = Join-Path $PSScriptRoot 'steam-vr-test.ps1'
        & $launcher -SteamRoot $SteamRoot -LogRoot $logRoot -SteamVrWaitSeconds $SteamVrWaitSeconds -GameWaitSeconds $GameWaitSeconds
        if (-not $?) { throw 'Steam VR launcher failed; inspect its output before retrying.' }
        $result = Write-Collection
        $result | ConvertTo-Json -Depth 8
    }
}
