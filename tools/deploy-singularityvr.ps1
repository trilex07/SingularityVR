[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [ValidateSet('Install', 'Rollback', 'Status')]
    [string]$Action = 'Install',
    [string]$SteamRoot = 'C:\Program Files (x86)\Steam',
    [string]$RepoRoot = '',
    [string]$ArtifactDir = '',
    [string]$XInputDllPath = '',
    [string]$BackupPath = '',
    [switch]$KeepIni,
    [switch]$StopGame
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-AbsolutePath([string]$Path) {
    if ([string]::IsNullOrWhiteSpace($Path)) { return $null }
    return [System.IO.Path]::GetFullPath($Path)
}

function Get-Sha256([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return $null }
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToUpperInvariant()
}

function Get-PeIdentity([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return $null }
    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 0x40 -or $bytes[0] -ne 0x4d -or $bytes[1] -ne 0x5a) {
        return [pscustomobject]@{ IsPe = $false; Machine = $null; OptionalMagic = $null; IsPe32 = $false }
    }
    $peOffset = [BitConverter]::ToInt32($bytes, 0x3c)
    if ($peOffset -lt 0 -or $peOffset + 0x1a -gt $bytes.Length) {
        return [pscustomobject]@{ IsPe = $false; Machine = $null; OptionalMagic = $null; IsPe32 = $false }
    }
    $signature = [BitConverter]::ToUInt32($bytes, $peOffset)
    $machine = [BitConverter]::ToUInt16($bytes, $peOffset + 4)
    $optionalMagic = [BitConverter]::ToUInt16($bytes, $peOffset + 24)
    [pscustomobject]@{
        IsPe = ($signature -eq 0x00004550)
        Machine = ('0x{0:X4}' -f $machine)
        OptionalMagic = ('0x{0:X4}' -f $optionalMagic)
        IsPe32 = ($signature -eq 0x00004550 -and $machine -eq 0x014c -and $optionalMagic -eq 0x010b)
    }
}

function Get-FileState([string]$Path) {
    $exists = Test-Path -LiteralPath $Path -PathType Leaf
    $pe = if ($exists -and [System.IO.Path]::GetExtension($Path) -ieq '.dll') { Get-PeIdentity $Path } else { $null }
    [pscustomobject]@{
        Path = $Path
        Exists = $exists
        Size = if ($exists) { (Get-Item -LiteralPath $Path).Length } else { $null }
        Sha256 = if ($exists) { Get-Sha256 $Path } else { $null }
        Pe32 = if ($pe) { $pe.IsPe32 } else { $null }
        Machine = if ($pe) { $pe.Machine } else { $null }
        OptionalMagic = if ($pe) { $pe.OptionalMagic } else { $null }
    }
}

function Stop-GameIfRequested {
    if (-not $StopGame) { return }
    $processes = @(Get-Process -Name 'Singularity' -ErrorAction SilentlyContinue)
    foreach ($process in $processes) {
        if ($PSCmdlet.ShouldProcess($process.ProcessName, 'Stop exact game process before deployment')) {
            Stop-Process -Id $process.Id -Force
        }
    }
}

function Copy-Verified([string]$Source, [string]$Target) {
    if (-not (Test-Path -LiteralPath $Source -PathType Leaf)) {
        throw "Missing deployment source: $Source"
    }
    $sourceHash = Get-Sha256 $Source
    $temp = "$Target.new.$PID"
    try {
        if ($PSCmdlet.ShouldProcess($Target, "Install $([System.IO.Path]::GetFileName($Source))")) {
            Copy-Item -LiteralPath $Source -Destination $temp -Force
            $tempHash = Get-Sha256 $temp
            if ($tempHash -ne $sourceHash) { throw "Staged copy hash mismatch for $Source" }
            Move-Item -LiteralPath $temp -Destination $Target -Force
            $targetHash = Get-Sha256 $Target
            if ($targetHash -ne $sourceHash) { throw "Installed file hash mismatch for $Target" }
        }
    } finally {
        if (Test-Path -LiteralPath $temp -PathType Leaf) { Remove-Item -LiteralPath $temp -Force }
    }
    [pscustomobject]@{ Source = $Source; Target = $Target; Sha256 = $sourceHash }
}

function New-Backup([string]$TargetDir, [string[]]$Targets) {
    $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
    $path = Join-Path $TargetDir $stamp
    if ($PSCmdlet.ShouldProcess($path, 'Create deployment backup')) {
        New-Item -ItemType Directory -Path $path -Force | Out-Null
        $entries = @()
        foreach ($target in $Targets) {
            if (Test-Path -LiteralPath $target -PathType Leaf) {
                $name = [System.IO.Path]::GetFileName($target)
                $backup = Join-Path $path $name
                Copy-Item -LiteralPath $target -Destination $backup -Force
                $entries += Get-FileState $backup
            }
        }
        $entries | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $path 'manifest.json') -Encoding UTF8
    }
    return $path
}

function Resolve-Source([string]$Name, [string]$Stage, [string]$Fallback) {
    $candidate = if ($Stage) { Join-Path $Stage $Name } else { $null }
    if ($candidate -and (Test-Path -LiteralPath $candidate -PathType Leaf)) { return $candidate }
    if ($Fallback -and (Test-Path -LiteralPath $Fallback -PathType Leaf)) { return $Fallback }
    return $null
}

$root = if ($RepoRoot) { Get-AbsolutePath $RepoRoot } else { (Resolve-Path (Join-Path $PSScriptRoot '..')).Path }
$gameBin = Join-Path $SteamRoot 'steamapps\common\Singularity\Binaries'
$sourceDll = Join-Path $root 'spikes\view_matrix\d3d9.dll'
$sourceIni = Join-Path $root 'spikes\view_matrix\SingularityVR.ini'
$stage = if ($ArtifactDir) { Get-AbsolutePath $ArtifactDir } else { Join-Path $root 'deployment\vr-recorder-20261009' }
$stageDll = Resolve-Source 'd3d9.dll' $stage $sourceDll
$stageIni = Resolve-Source 'SingularityVR.ini' $stage $sourceIni
$targetDll = Join-Path $gameBin 'd3d9.dll'
$targetIni = Join-Path $gameBin 'SingularityVR.ini'

if ($Action -eq 'Status') {
    $state = @(
        Get-FileState $targetDll
        Get-FileState $targetIni
    )
    $state | Format-Table -AutoSize
    if (Test-Path -LiteralPath $targetDll -PathType Leaf) {
        $pe = Get-PeIdentity $targetDll
        Write-Host ("DLL PE32: {0} (machine {1}, optional {2})" -f $pe.IsPe32, $pe.Machine, $pe.OptionalMagic)
    }
    exit 0
}

if (-not (Test-Path -LiteralPath $gameBin -PathType Container)) {
    throw "Singularity Binaries directory not found: $gameBin"
}

if ($Action -eq 'Rollback') {
    if (-not $BackupPath) { throw 'Rollback requires -BackupPath pointing to a timestamped backup directory.' }
    $backup = Get-AbsolutePath $BackupPath
    $manifest = Join-Path $backup 'manifest.json'
    if (-not (Test-Path -LiteralPath $manifest -PathType Leaf)) { throw "Backup manifest not found: $manifest" }
    Stop-GameIfRequested
    foreach ($file in @('d3d9.dll', 'SingularityVR.ini', 'xinput1_3.dll')) {
        $src = Join-Path $backup $file
        $dst = Join-Path $gameBin $file
        if (Test-Path -LiteralPath $src -PathType Leaf) { Copy-Verified $src $dst | Out-Null }
    }
    Write-Host "Rollback complete from $backup"
    exit 0
}

if (-not $stageDll) { throw 'No d3d9.dll source found in the selected artifact directory or source tree.' }
if (-not $stageIni -and -not $KeepIni) { throw 'No SingularityVR.ini source found; use -KeepIni only if preserving the installed INI is intentional.' }
$pe = Get-PeIdentity $stageDll
if (-not $pe.IsPe32) { throw "The deployment DLL is not PE32 x86: $stageDll" }

$targets = @($targetDll)
if (-not $KeepIni) { $targets += $targetIni }
$xinputSource = $null
if ($XInputDllPath) {
    $xinputSource = Get-AbsolutePath $XInputDllPath
    if (-not (Test-Path -LiteralPath $xinputSource -PathType Leaf)) { throw "XInput DLL not found: $xinputSource" }
    $targets += (Join-Path $gameBin ([System.IO.Path]::GetFileName($xinputSource)))
}
$backupRoot = Join-Path $root 'deployment\install-backups'
New-Item -ItemType Directory -Path $backupRoot -Force | Out-Null
Stop-GameIfRequested
$backup = New-Backup $backupRoot $targets
$installed = @()
$installed += Copy-Verified $stageDll $targetDll
if (-not $KeepIni) { $installed += Copy-Verified $stageIni $targetIni }
if ($xinputSource) { $installed += Copy-Verified $xinputSource (Join-Path $gameBin ([System.IO.Path]::GetFileName($xinputSource))) }

$reportDir = Join-Path $root 'deployment\install-reports'
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
$report = [pscustomobject]@{
    Action = 'Install'
    Timestamp = (Get-Date).ToString('o')
    GameBin = $gameBin
    BackupPath = $backup
    Pe32 = $pe.IsPe32
    Installed = @($installed)
    XInput = if ($xinputSource) { $xinputSource } else { 'not supplied; no xinput DLL copied' }
    FinalState = @(
        Get-FileState $targetDll
        Get-FileState $targetIni
    )
}
$reportPath = Join-Path $reportDir ((Get-Date -Format 'yyyyMMdd-HHmmss') + '.json')
$report | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $reportPath -Encoding UTF8
Write-Host "SingularityVR deployment complete."
Write-Host "Game directory: $gameBin"
Write-Host "Backup: $backup"
Write-Host "Report: $reportPath"
Write-Host ("d3d9.dll SHA-256: {0}" -f (Get-Sha256 $targetDll))
if (-not $xinputSource) { Write-Warning 'No xinput DLL was supplied or present in the current repository; no xinput file was copied.' }
