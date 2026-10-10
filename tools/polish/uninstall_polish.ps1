<#
.SYNOPSIS
    Uninstalls Polish localization and dubbing for SingularityVR

.DESCRIPTION
    This script safely removes Polish localization and restores original files
    from the backup created during installation.

.NOTES
    Run from: C:\Users\Trilex\vr-mods\SingularityVR-production
    Requires: Administrator privileges for game directory access
#>

param(
    [Parameter(Mandatory=$false)]
    [string]$GamePath = "C:\Program Files (x86)\Steam\steamapps\common\Singularity",
    
    [Parameter(Mandatory=$false)]
    [string]$BackupPath,
    
    [Parameter(Mandatory=$false)]
    [switch]$Force,
    
    [Parameter(Mandatory=$false)]
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$GameBinaries = Join-Path $GamePath "Binaries"
$GameCookedPC = Join-Path $GamePath "RvGame\CookedPC"

function Write-Log {
    param([string]$Message, [string]$Level = "INFO")
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    Write-Host "[$timestamp] [$Level] $Message"
}

# Find backup directory
if (-not $BackupPath) {
    $backups = Get-ChildItem -Path (Join-Path $GameBinaries "polish-backup-*") -Directory -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending
    if ($backups.Count -eq 0) {
        throw "No backup directory found. Specify -BackupPath manually."
    }
    $BackupPath = $backups[0].FullName
    Write-Log "Using latest backup: $BackupPath"
}

if (-not (Test-Path $BackupPath)) {
    throw "Backup path not found: $BackupPath"
}

Write-Log "=== SingularityVR Polish Localization Uninstaller ==="
Write-Log "Game path: $GamePath"
Write-Log "Backup path: $BackupPath"
Write-Log "Dry run: $DryRun"

# Restore Coalesced_INT.bin
$coalescedBackup = Join-Path $BackupPath "Coalesced_INT.bin"
$coalescedTarget = Join-Path $GameCookedPC "Coalesced_INT.bin"
if (Test-Path $coalescedBackup) {
    Write-Log "Restoring Coalesced_INT.bin"
    if (-not $DryRun) {
        Copy-Item -LiteralPath $coalescedBackup -Destination $coalescedTarget -Force
    }
} else {
    Write-Log "No Coalesced_INT.bin in backup, skipping" -Level "WARN"
}

# Restore any other backed up files
$backupFiles = Get-ChildItem -Path $BackupPath -File -Recurse | Where-Object { $_.Name -ne "Coalesced_INT.bin" }
foreach ($file in $backupFiles) {
    $relPath = $file.FullName.Substring($BackupPath.Length + 1)
    $targetPath = Join-Path $GameCookedPC $relPath
    Write-Log "Restoring: $relPath"
    if (-not $DryRun) {
        New-Item -ItemType Directory -Path (Split-Path $targetPath) -Force | Out-Null
        Copy-Item -LiteralPath $file.FullName -Destination $targetPath -Force
    }
}

# Remove Polish mod assets
$polishDirs = @(
    Join-Path $GameBinaries "vr-assets\localization\pl"
    Join-Path $GameBinaries "vr-assets\audio\polish"
)

foreach ($dir in $polishDirs) {
    if (Test-Path $dir) {
        Write-Log "Removing Polish mod directory: $dir"
        if (-not $DryRun) {
            Remove-Item -LiteralPath $dir -Recurse -Force
        }
    }
}

# Remove manifest
$manifestPath = Join-Path $GameBinaries "vr-assets\polish_install_manifest.json"
if (Test-Path $manifestPath) {
    Write-Log "Removing install manifest"
    if (-not $DryRun) {
        Remove-Item -LiteralPath $manifestPath -Force
    }
}

# Update INI - disable Polish
$iniPath = Join-Path $GameBinaries "SingularityVR.ini"
if (Test-Path $iniPath) {
    $iniContent = Get-Content -Path $iniPath -Raw
    if ($iniContent -match 'PolishLanguage=1') {
        Write-Log "Disabling Polish in INI"
        if (-not $DryRun) {
            $newContent = $iniContent -replace 'PolishLanguage=1', 'PolishLanguage=0'
            Set-Content -Path $iniPath -Value $newContent -Encoding UTF8
        }
    }
}

# Remove SingularityVfx.dll (VFX plugin)
$vfxDllPath = Join-Path $GameBinaries "SingularityVfx.dll"
if (Test-Path $vfxDllPath) {
    Write-Log "Removing SingularityVfx.dll (VFX plugin)"
    if (-not $DryRun) {
        Remove-Item -LiteralPath $vfxDllPath -Force
    }
}

# Clean up empty vr-assets directory
$vrAssetsDir = Join-Path $GameBinaries "vr-assets"
if (Test-Path $vrAssetsDir) {
    $remaining = Get-ChildItem -Path $vrAssetsDir -Recurse -ErrorAction SilentlyContinue
    if ($remaining.Count -eq 0) {
        Write-Log "Removing empty vr-assets directory"
        if (-not $DryRun) {
            Remove-Item -LiteralPath $vrAssetsDir -Recurse -Force
        }
    }
}

Write-Log "=== UNINSTALLATION COMPLETE ==="
if ($DryRun) {
    Write-Log "DRY RUN COMPLETE - No changes were made" -Level "WARN"
}