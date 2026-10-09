[CmdletBinding(SupportsShouldProcess)]
param(
    [ValidateSet('Verify', 'Backup', 'Rollback')]
    [string]$Action = 'Verify',
    [string]$SteamRoot = '',
    [string]$Artifact = '',
    [string]$ExpectedSha256 = '',
    [string]$BackupDirectory = '',
    [switch]$ConfirmRollback
)

# This is a guard, not a deployment tool. There is intentionally no Deploy
# action. Backup reads the Steam install; Rollback is an explicit, confirmed
# mutation owned by the main agent.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Get-PeIdentity([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "PE file missing: $Path" }
    $bytes = [IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 0x40 -or $bytes[0] -ne 0x4d -or $bytes[1] -ne 0x5a) { throw "Not an MZ file: $Path" }
    $offset = [BitConverter]::ToInt32($bytes, 0x3c)
    if ($offset -lt 0 -or $offset + 0x1a -gt $bytes.Length -or [BitConverter]::ToUInt32($bytes, $offset) -ne 0x4550) { throw "Not a PE file: $Path" }
    $machine = [BitConverter]::ToUInt16($bytes, $offset + 4)
    $magic = [BitConverter]::ToUInt16($bytes, $offset + 24)
    [ordered]@{ machine = ('0x{0:X4}' -f $machine); optionalHeaderMagic = ('0x{0:X4}' -f $magic); x86Pe32 = ($machine -eq 0x014c -and $magic -eq 0x010b) }
}

function Get-Sha([string]$Path) { (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToUpperInvariant() }

function Get-GameBin([string]$Root) {
    if (-not $Root) { throw 'SteamRoot is required.' }
    $root = [IO.Path]::GetFullPath($Root)
    $path = [IO.Path]::GetFullPath((Join-Path $root 'steamapps\common\Singularity\Binaries'))
    if (-not $path.StartsWith($root.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Resolved game path escaped SteamRoot.' }
    if (-not (Test-Path -LiteralPath $path -PathType Container)) { throw "Singularity Binaries missing: $path" }
    return $path
}

function Verify-Artifact([string]$Path) {
    $pe = Get-PeIdentity $Path
    $sha = Get-Sha $Path
    $result = [ordered]@{ action = 'Verify'; artifact = [IO.Path]::GetFullPath($Path); sha256 = $sha; pe = $pe; expectedSha256 = if ($ExpectedSha256) { $ExpectedSha256.ToUpperInvariant() } else { $null }; shaMatches = if ($ExpectedSha256) { $sha -eq $ExpectedSha256.ToUpperInvariant() } else { $true }; pass = [bool]($pe.x86Pe32 -and (!$ExpectedSha256 -or $sha -eq $ExpectedSha256.ToUpperInvariant())) }
    $result | ConvertTo-Json -Depth 5
    if (-not $result.pass) { exit 2 }
}

if ($Action -eq 'Verify') {
    if (-not $Artifact) { throw 'Verify requires -Artifact.' }
    Verify-Artifact $Artifact
    exit 0
}

$gameBin = Get-GameBin $SteamRoot
$files = @('d3d9.dll', 'SingularityVR.ini', 'SingularityVfx.dll')

if ($Action -eq 'Backup') {
    if (-not $BackupDirectory) { throw 'Backup requires -BackupDirectory.' }
    $backup = [IO.Path]::GetFullPath($BackupDirectory)
    if (Test-Path -LiteralPath $backup) { throw "Backup directory already exists: $backup" }
    New-Item -ItemType Directory -Force -Path $backup | Out-Null
    $entries = @()
    foreach ($name in $files) {
        $source = Join-Path $gameBin $name
        if (Test-Path -LiteralPath $source -PathType Leaf) {
            $target = Join-Path $backup $name
            Copy-Item -LiteralPath $source -Destination $target -Force
            $entries += [ordered]@{ name = $name; present = $true; sha256 = Get-Sha $target; bytes = (Get-Item -LiteralPath $target).Length }
        } else {
            $entries += [ordered]@{ name = $name; present = $false; sha256 = $null; bytes = 0 }
        }
    }
    [ordered]@{ schema = 'singularityvr.steam-backup.v1'; createdAt = (Get-Date).ToString('o'); gameBin = $gameBin; files = $entries } | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $backup 'manifest.json') -Encoding UTF8
    [ordered]@{ action = 'Backup'; backupDirectory = $backup; manifest = Join-Path $backup 'manifest.json'; pass = $true } | ConvertTo-Json -Depth 6
    exit 0
}

if (-not $ConfirmRollback) { throw 'Rollback is a mutating action. Re-run with -ConfirmRollback.' }
if (-not $BackupDirectory) { throw 'Rollback requires -BackupDirectory.' }
$backup = [IO.Path]::GetFullPath($BackupDirectory)
$manifestPath = Join-Path $backup 'manifest.json'
if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) { throw "Backup manifest missing: $manifestPath" }
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if ($manifest.schema -ne 'singularityvr.steam-backup.v1') { throw 'Unsupported backup manifest schema.' }
foreach ($entry in @($manifest.files)) {
    $target = Join-Path $gameBin $entry.name
    $source = Join-Path $backup $entry.name
    if ($entry.present) {
        if (-not (Test-Path -LiteralPath $source -PathType Leaf)) { throw "Backup payload missing: $source" }
        $backupSha = Get-Sha $source
        if ($backupSha -ne $entry.sha256) { throw "Backup hash changed: $source" }
        if ($PSCmdlet.ShouldProcess($target, 'restore exact backup')) { Copy-Item -LiteralPath $source -Destination $target -Force }
    } elseif (Test-Path -LiteralPath $target -PathType Leaf) {
        if ($PSCmdlet.ShouldProcess($target, 'remove file absent from backup')) { Remove-Item -LiteralPath $target -Force }
    }
}
$checks = @($manifest.files | ForEach-Object {
    $target = Join-Path $gameBin $_.name
    [ordered]@{ name = $_.name; expectedPresent = [bool]$_.present; actualPresent = (Test-Path -LiteralPath $target -PathType Leaf); sha256 = if (Test-Path -LiteralPath $target -PathType Leaf) { Get-Sha $target } else { $null }; expectedSha256 = $_.sha256 }
})
$pass = [bool](@($checks | Where-Object { $_.actualPresent -ne $_.expectedPresent -or ($_.expectedPresent -and $_.sha256 -ne $_.expectedSha256) }).Count -eq 0)
[ordered]@{ action = 'Rollback'; backupDirectory = $backup; checks = $checks; pass = $pass } | ConvertTo-Json -Depth 6
if (-not $pass) { exit 2 }
