[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [ValidateSet('Install', 'Rollback', 'Status')]
    [string]$Action = 'Install',
    [string]$SteamRoot = 'C:\Program Files (x86)\Steam',
    [string]$RepoRoot = '',
    [string]$ArtifactDir = '',
    [string]$AssetsRoot = '',
    [string]$AssetsLocalRoot = '',
    [ValidateSet('A', 'B')]
    [string]$TextureVariant = 'B',
    [string]$BackupPath = '',
    [switch]$KeepIni,
    [switch]$DllOnly,
    [switch]$SkipTextures,
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
        $metadata = [pscustomobject]@{
            Files = @($Targets | ForEach-Object {
                [pscustomobject]@{
                    Name = [System.IO.Path]::GetFileName($_)
                    Existed = (Test-Path -LiteralPath $_ -PathType Leaf)
                    Sha256 = if (Test-Path -LiteralPath $_ -PathType Leaf) { Get-Sha256 $_ } else { $null }
                }
            })
            Directories = @()
        }
        $metadata | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $path 'backup_metadata.json') -Encoding UTF8
    }
    return $path
}

function Add-DirectoryBackup([string]$Backup, [string]$Target) {
    $name = [System.IO.Path]::GetFileName($Target.TrimEnd('\'))
    $destination = Join-Path $Backup $name
    $existed = Test-Path -LiteralPath $Target -PathType Container
    if ($existed -and $PSCmdlet.ShouldProcess($destination, 'Back up texture directory')) {
        Copy-Item -LiteralPath $Target -Destination $destination -Recurse -Force
    }
    [pscustomobject]@{ Name = $name; Existed = $existed }
}

function Set-DirectoryMetadata([string]$Backup, [object[]]$Directories) {
    $metadataPath = Join-Path $Backup 'backup_metadata.json'
    $metadata = if (Test-Path -LiteralPath $metadataPath -PathType Leaf) {
        Get-Content -LiteralPath $metadataPath -Raw | ConvertFrom-Json
    } else {
        [pscustomobject]@{ Files = @(); Directories = @() }
    }
    $metadata.Directories = @($Directories)
    $metadata | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $metadataPath -Encoding UTF8
}

function Copy-Directory([string]$Source, [string]$Target) {
    if (-not (Test-Path -LiteralPath $Source -PathType Container)) { throw "Missing directory source: $Source" }
    if ($PSCmdlet.ShouldProcess($Target, "Install texture directory $([System.IO.Path]::GetFileName($Source))")) {
        New-Item -ItemType Directory -Path $Target -Force | Out-Null
        Get-ChildItem -LiteralPath $Target -File -Filter '*.dds' -ErrorAction SilentlyContinue | Remove-Item -Force
        Get-ChildItem -LiteralPath $Source -File -Filter '*.dds' | ForEach-Object {
            Copy-Verified $_.FullName (Join-Path $Target $_.Name) | Out-Null
        }
    }
}

function Get-TextureSources([string]$LocalRoot) {
    $upscaled = Join-Path $LocalRoot 'textures\upscaled'
    if (-not (Test-Path -LiteralPath $upscaled -PathType Container)) {
        throw "Upscaled texture directory not found: $upscaled"
    }
    $suffix = if ($TextureVariant -eq 'B') { '__B_ibp' } else { '__A_lanczos' }
    $strip = "$suffix.dds"
    $sources = @(Get-ChildItem -LiteralPath $upscaled -File -Filter "*$strip")
    if ($sources.Count -eq 0) { throw "No $TextureVariant texture candidates found in $upscaled" }
    return @($sources | ForEach-Object {
        $base = $_.Name.Substring(0, $_.Name.Length - $strip.Length)
        [pscustomobject]@{ Source = $_.FullName; Name = "$base.dds" }
    })
}

function Resolve-Source([string]$Name, [string]$Stage, [string]$Fallback) {
    $candidate = if ($Stage) { Join-Path $Stage $Name } else { $null }
    if ($candidate -and (Test-Path -LiteralPath $candidate -PathType Leaf)) { return $candidate }
    if ($Fallback -and (Test-Path -LiteralPath $Fallback -PathType Leaf)) { return $Fallback }
    return $null
}

$root = if ($RepoRoot) { Get-AbsolutePath $RepoRoot } else { (Resolve-Path (Join-Path $PSScriptRoot '..')).Path }
$steamRootAbs = Get-AbsolutePath $SteamRoot
$gameBin = Join-Path $steamRootAbs 'steamapps\common\Singularity\Binaries'
$sourceDll = Join-Path $root 'spikes\view_matrix\d3d9.dll'
$sourceIni = Join-Path $root 'spikes\view_matrix\SingularityVR.ini'
$stage = if ($ArtifactDir) { Get-AbsolutePath $ArtifactDir } else { Join-Path $root 'deployment\vr-recorder-20261009' }
# The source tree is authoritative for the normal path. An explicitly supplied
# -ArtifactDir is allowed to override it for a deliberate rollback/candidate install.
$stageDll = if ($ArtifactDir) { Resolve-Source 'd3d9.dll' $stage $sourceDll } else { Resolve-Source 'd3d9.dll' $null $sourceDll }
$stageIni = if ($ArtifactDir) { Resolve-Source 'SingularityVR.ini' $stage $sourceIni } else { Resolve-Source 'SingularityVR.ini' $null $sourceIni }
$assets = if ($AssetsRoot) { Get-AbsolutePath $AssetsRoot } else { Join-Path (Split-Path $root -Parent) 'SingularityVR-assets' }
$assetsLocal = if ($AssetsLocalRoot) { Get-AbsolutePath $AssetsLocalRoot } else { Join-Path (Split-Path $root -Parent) 'SingularityVR-assets-local' }
$dinputSource = Join-Path $assets 'asset-tools\texture-injector\dinput8.dll'
$dinputStable = Join-Path $root 'deployment\install-backups\20261009-114517\dinput8.dll'
if (Test-Path -LiteralPath $dinputSource -PathType Leaf) {
    $dinputHash = (Get-FileHash -LiteralPath $dinputSource -Algorithm SHA256).Hash.ToUpperInvariant()
    # 37376-byte asset-tools build (B810…) has crashed at startup with current d3d9; keep 54784 (939E…).
    if ($dinputHash -eq 'B810FA5256287A293382FF30DA9D0CC906D317F3570BCC7F3CA920B058AC9FDB' -and
        (Test-Path -LiteralPath $dinputStable -PathType Leaf)) {
        Write-Warning 'dinput8: using stable 54784-byte backup instead of 37376-byte asset-tools build.'
        $dinputSource = $dinputStable
    }
}
$textureIniSource = Join-Path $assets 'asset-tools\texture-injector\texture_override.ini'
$dinputOrigSource = Join-Path $env:windir 'SysWOW64\dinput8.dll'
$targetDll = Join-Path $gameBin 'd3d9.dll'
$targetIni = Join-Path $gameBin 'SingularityVR.ini'
$targetDinput = Join-Path $gameBin 'dinput8.dll'
$targetDinputOrig = Join-Path $gameBin 'dinput8_orig.dll'
$targetTextureIni = Join-Path $gameBin 'texture_override.ini'
$targetTextures = Join-Path $gameBin 'texture-override'

if ($Action -eq 'Status') {
    $state = @(
        Get-FileState $targetDll
        Get-FileState $targetIni
        Get-FileState $targetDinput
        Get-FileState $targetDinputOrig
        Get-FileState $targetTextureIni
    )
    $state | Format-Table -AutoSize
    if (Test-Path -LiteralPath $targetDll -PathType Leaf) {
        $pe = Get-PeIdentity $targetDll
        Write-Host ("DLL PE32: {0} (machine {1}, optional {2})" -f $pe.IsPe32, $pe.Machine, $pe.OptionalMagic)
    }
    if (Test-Path -LiteralPath $targetTextures -PathType Container) {
        $textureCount = @(Get-ChildItem -LiteralPath $targetTextures -File -Filter '*.dds').Count
        Write-Host "Texture override DDS files: $textureCount"
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
    $metadataPath = Join-Path $backup 'backup_metadata.json'
    if (Test-Path -LiteralPath $metadataPath -PathType Leaf) {
        $metadata = Get-Content -LiteralPath $metadataPath -Raw | ConvertFrom-Json
        foreach ($entry in @($metadata.Files)) {
            $src = Join-Path $backup $entry.Name
            $dst = Join-Path $gameBin $entry.Name
            if ($entry.Existed) {
                if (Test-Path -LiteralPath $src -PathType Leaf) { Copy-Verified $src $dst | Out-Null }
            } elseif (Test-Path -LiteralPath $dst -PathType Leaf) {
                if ($PSCmdlet.ShouldProcess($dst, 'Remove file that did not exist before deployment')) { Remove-Item -LiteralPath $dst -Force }
            }
        }
        foreach ($entry in @($metadata.Directories)) {
            $dst = Join-Path $gameBin $entry.Name
            if (Test-Path -LiteralPath $dst -PathType Container) {
                if ($PSCmdlet.ShouldProcess($dst, 'Remove installed texture directory before rollback')) { Remove-Item -LiteralPath $dst -Recurse -Force }
            }
            $src = Join-Path $backup $entry.Name
            if ($entry.Existed -and (Test-Path -LiteralPath $src -PathType Container)) {
                if ($PSCmdlet.ShouldProcess($dst, 'Restore texture directory')) { Copy-Item -LiteralPath $src -Destination $dst -Recurse -Force }
            }
        }
    } else {
        foreach ($file in @('d3d9.dll', 'SingularityVR.ini', 'dinput8.dll', 'dinput8_orig.dll', 'texture_override.ini')) {
            $src = Join-Path $backup $file
            $dst = Join-Path $gameBin $file
            if (Test-Path -LiteralPath $src -PathType Leaf) { Copy-Verified $src $dst | Out-Null }
        }
    }
    Write-Host "Rollback complete from $backup"
    exit 0
}

if (-not $stageDll) { throw 'No d3d9.dll source found in the selected artifact directory or source tree.' }
if (-not $DllOnly -and -not $stageIni -and -not $KeepIni) { throw 'No SingularityVR.ini source found; use -KeepIni only if preserving the installed INI is intentional.' }
if (-not $DllOnly -and -not (Test-Path -LiteralPath $dinputSource -PathType Leaf)) { throw "Texture injector dinput8.dll not found: $dinputSource" }
if (-not $DllOnly -and -not (Test-Path -LiteralPath $textureIniSource -PathType Leaf)) { throw "Texture injector config not found: $textureIniSource" }
if (-not $DllOnly -and -not (Test-Path -LiteralPath $dinputOrigSource -PathType Leaf)) { throw "System dinput8.dll not found: $dinputOrigSource" }
$pe = Get-PeIdentity $stageDll
if (-not $pe.IsPe32) { throw "The deployment DLL is not PE32 x86: $stageDll" }
$dinputPe = if ($DllOnly) { $null } else { Get-PeIdentity $dinputSource }
if (-not $DllOnly -and -not $dinputPe.IsPe32) { throw "The texture injector dinput8.dll is not PE32 x86: $dinputSource" }

$targets = @($targetDll)
if (-not $DllOnly) {
    $targets += @($targetDinput, $targetDinputOrig, $targetTextureIni)
    if (-not $KeepIni) { $targets += $targetIni }
}
$textureSources = @()
if (-not $DllOnly -and -not $SkipTextures) { $textureSources = @(Get-TextureSources $assetsLocal) }
$backupRoot = Join-Path $root 'deployment\install-backups'
New-Item -ItemType Directory -Path $backupRoot -Force | Out-Null
Stop-GameIfRequested
$backup = New-Backup $backupRoot $targets
$directoryBackups = @()
if (-not $DllOnly -and -not $SkipTextures) { $directoryBackups += Add-DirectoryBackup $backup $targetTextures }
Set-DirectoryMetadata $backup $directoryBackups
$installed = @()
$installed += Copy-Verified $stageDll $targetDll
if (-not $DllOnly) {
    if (-not $KeepIni) { $installed += Copy-Verified $stageIni $targetIni }
    $installed += Copy-Verified $dinputSource $targetDinput
    $installed += Copy-Verified $dinputOrigSource $targetDinputOrig
    $installed += Copy-Verified $textureIniSource $targetTextureIni
}
if (-not $DllOnly -and -not $SkipTextures) {
    New-Item -ItemType Directory -Path $targetTextures -Force | Out-Null
    Get-ChildItem -LiteralPath $targetTextures -File -Filter '*.dds' -ErrorAction SilentlyContinue | Remove-Item -Force
    foreach ($texture in $textureSources) {
        $installed += Copy-Verified $texture.Source (Join-Path $targetTextures $texture.Name)
    }
}

$reportDir = Join-Path $root 'deployment\install-reports'
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
$report = [pscustomobject]@{
    Action = 'Install'
    Timestamp = (Get-Date).ToString('o')
    GameBin = $gameBin
    BackupPath = $backup
    Pe32 = $pe.IsPe32
    DllOnly = [bool]$DllOnly
    DInput8Pe32 = if ($dinputPe) { $dinputPe.IsPe32 } else { $null }
    Installed = @($installed)
    TextureVariant = if ($DllOnly -or $SkipTextures) { 'skipped' } else { $TextureVariant }
    TextureCount = if ($DllOnly -or $SkipTextures) { 0 } else { $textureSources.Count }
    FinalState = @(
        Get-FileState $targetDll
        Get-FileState $targetIni
        Get-FileState $targetDinput
        Get-FileState $targetDinputOrig
        Get-FileState $targetTextureIni
    )
}
$reportPath = Join-Path $reportDir ((Get-Date -Format 'yyyyMMdd-HHmmss') + '.json')
$report | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $reportPath -Encoding UTF8
Write-Host "SingularityVR deployment complete."
Write-Host "Game directory: $gameBin"
Write-Host "Backup: $backup"
Write-Host "Report: $reportPath"
Write-Host ("d3d9.dll SHA-256: {0}" -f (Get-Sha256 $targetDll))
if (-not $DllOnly) { Write-Host ("dinput8.dll SHA-256: {0}" -f (Get-Sha256 $targetDinput)) }
if (-not $DllOnly -and -not $SkipTextures) { Write-Host ("Texture variant {0}: {1} DDS file(s)" -f $TextureVariant, $textureSources.Count) }
