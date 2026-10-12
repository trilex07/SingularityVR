[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [ValidateSet('Preflight', 'Install', 'Restore', 'Status')]
    [string]$Action = 'Preflight',
    [string]$SteamRoot = 'C:\Program Files (x86)\Steam',
    [string]$CandidatePath = 'C:\Users\Trilex\vr-mods\pause-re-lab\native-rtt-dedicated-v1\UI_SinglePlayer_SF.xxx',
    [string]$BackupPath = ''
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-Sha256([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return $null }
    (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToUpperInvariant()
}

function Test-FileUnlocked([string]$Path) {
    try {
        $stream = [System.IO.File]::Open($Path, [System.IO.FileMode]::Open,
            [System.IO.FileAccess]::ReadWrite, [System.IO.FileShare]::None)
        $stream.Dispose()
        return $true
    } catch { return $false }
}

function Copy-Verified([string]$Source, [string]$Target) {
    if (-not (Test-Path -LiteralPath $Source -PathType Leaf)) { throw "Missing source: $Source" }
    if (-not (Test-FileUnlocked $Target)) { throw "Target is locked; do not replace it: $Target" }
    $sourceHash = Get-Sha256 $Source
    $temp = "$Target.pause-rtt-$PID.new"
    try {
        if ($PSCmdlet.ShouldProcess($Target, "copy verified pause RTT package from $Source")) {
            Copy-Item -LiteralPath $Source -Destination $temp -Force
            if ((Get-Sha256 $temp) -ne $sourceHash) { throw "Staged hash mismatch: $temp" }
            Move-Item -LiteralPath $temp -Destination $Target -Force
            if ((Get-Sha256 $Target) -ne $sourceHash) { throw "Installed hash mismatch: $Target" }
        }
    } finally {
        if (Test-Path -LiteralPath $temp -PathType Leaf) { Remove-Item -LiteralPath $temp -Force }
    }
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$gameRoot = Join-Path ([System.IO.Path]::GetFullPath($SteamRoot)) 'steamapps\common\Singularity'
$gameBin = Join-Path $gameRoot 'Binaries'
$package = Join-Path $gameRoot 'RvGame\CookedPC\UI_SinglePlayer_SF.xxx'
$dll = Join-Path $gameBin 'd3d9.dll'
$ini = Join-Path $gameBin 'SingularityVR.ini'
$backupRoot = Join-Path $repoRoot 'deployment\pause-rtt-package-backups'
$expectedDll = '2FE5011C20DFEAAEDFFFFE58CD39E92A9570867F344649B00C0038AD97B39A47'
$expectedCandidate = '5B2EA469A3EE06FF154AC78BD58604E8034970D4837F289E5C22503C6A673CDE'

function Get-State {
    [ordered]@{
        timestamp = (Get-Date).ToString('o')
        gameRunning = @(Get-Process -Name Singularity -ErrorAction SilentlyContinue | ForEach-Object { $_.Id })
        package = [ordered]@{ path = $package; sha256 = Get-Sha256 $package }
        dll = [ordered]@{ path = $dll; sha256 = Get-Sha256 $dll; acceptedBaseline = ((Get-Sha256 $dll) -eq $expectedDll) }
        ini = [ordered]@{ path = $ini; sha256 = Get-Sha256 $ini }
        candidate = [ordered]@{ path = $CandidatePath; sha256 = Get-Sha256 $CandidatePath; expected = $expectedCandidate; valid = ((Get-Sha256 $CandidatePath) -eq $expectedCandidate) }
    }
}

if ($Action -eq 'Status' -or $Action -eq 'Preflight') {
    $state = Get-State
    $state | ConvertTo-Json -Depth 5
    if ($Action -eq 'Preflight') {
        if ($state.gameRunning.Count -ne 0) { throw "Singularity is running (PID(s): $($state.gameRunning -join ', ')); deployment is forbidden." }
        if (-not $state.dll.acceptedBaseline) { throw "Refusing package-only Gate 1: installed d3d9.dll is not accepted baseline $expectedDll." }
        if (-not $state.candidate.valid) { throw "Candidate SHA-256 differs from expected dedicated RTT artifact." }
        if (-not $state.package.sha256 -or -not $state.ini.sha256) { throw 'Required installed UI package or INI is missing.' }
    }
    exit 0
}

if ($Action -eq 'Install') {
    $state = Get-State
    if ($state.gameRunning.Count -ne 0) { throw "Singularity is running (PID(s): $($state.gameRunning -join ', ')); deployment is forbidden." }
    if (-not $state.dll.acceptedBaseline) { throw "Installed d3d9.dll is not accepted baseline $expectedDll." }
    if (-not $state.candidate.valid) { throw 'Candidate hash does not match the reviewed dedicated RTT artifact.' }
    if (-not (Test-FileUnlocked $package)) { throw "UI package is locked; deployment is forbidden: $package" }
    New-Item -ItemType Directory -Force -Path $backupRoot | Out-Null
    $backup = Join-Path $backupRoot (Get-Date -Format 'yyyyMMdd-HHmmss')
    if ($PSCmdlet.ShouldProcess($backup, 'create pause RTT package backup')) {
        New-Item -ItemType Directory -Path $backup -Force | Out-Null
        foreach ($source in @($package, $dll, $ini)) { Copy-Item -LiteralPath $source -Destination (Join-Path $backup ([IO.Path]::GetFileName($source))) -Force }
        $manifest = [ordered]@{
            createdAt = (Get-Date).ToString('o'); candidate = $CandidatePath; candidateSha256 = $state.candidate.sha256
            files = @($package, $dll, $ini | ForEach-Object { [ordered]@{ name = [IO.Path]::GetFileName($_); sourcePath = $_; sha256 = Get-Sha256 $_ } })
        }
        $manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $backup 'manifest.json') -Encoding UTF8
    }
    Copy-Verified $CandidatePath $package
    $after = Get-State
    if ($after.package.sha256 -ne $expectedCandidate -or -not $after.dll.acceptedBaseline) { throw 'Post-install identity check failed.' }
    [ordered]@{ action = 'Install'; backup = $backup; state = $after } | ConvertTo-Json -Depth 6
    exit 0
}

if (-not $BackupPath) { throw 'Restore requires -BackupPath.' }
$backup = [IO.Path]::GetFullPath($BackupPath)
$manifestPath = Join-Path $backup 'manifest.json'
if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) { throw "Backup manifest not found: $manifestPath" }
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
$running = @(Get-Process -Name Singularity -ErrorAction SilentlyContinue)
if ($running.Count -ne 0) { throw "Singularity is running (PID(s): $($running.Id -join ', ')); restoration is forbidden." }
foreach ($entry in @($manifest.files)) {
    $source = Join-Path $backup $entry.name
    $target = $entry.sourcePath
    Copy-Verified $source $target
    if ((Get-Sha256 $target) -ne $entry.sha256) { throw "Rollback hash mismatch: $target" }
}
[ordered]@{ action = 'Restore'; backup = $backup; restored = Get-State } | ConvertTo-Json -Depth 6
