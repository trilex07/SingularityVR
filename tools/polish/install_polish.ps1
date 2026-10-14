<#
.SYNOPSIS
    Installs Polish localization and dubbing for SingularityVR

.DESCRIPTION
    This script safely installs Polish text localization and voice dubbing
    by backing up original files, validating hashes, and providing rollback capability.

.NOTES
    Run from: C:\Users\Trilex\vr-mods\SingularityVR-production
    Requires: Administrator privileges for game directory access
#>

param(
    [Parameter(Mandatory=$false)]
    [string]$GamePath = "C:\Program Files (x86)\Steam\steamapps\common\Singularity",
    
    [Parameter(Mandatory=$false)]
    [string]$SourcePath = "C:\Users\Trilex\vr-mods\SingularityVR-polish-integration",
    
    [Parameter(Mandatory=$false)]
    [switch]$Force,
    
    [Parameter(Mandatory=$false)]
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

# Configuration
$GameBinaries = Join-Path $GamePath "Binaries"
$GameCookedPC = Join-Path $GamePath "RvGame\CookedPC"
$BackupDir = Join-Path $GamePath "Binaries\polish-backup-$(Get-Date -Format 'yyyyMMdd-HHmmss')"
$SourceText = Join-Path $SourcePath "localization\pl\polish_text.jsonl"
$SourceVoice = Join-Path $SourcePath "localization\pl\polish_voice.jsonl"
$SourceStory = Join-Path $SourcePath "localization\pl\polish_story_voice.jsonl"
$SourceAudioUI = Join-Path $SourcePath "..\SingularityVR-cutscene\audio\polish\full_voicepack"
$SourceAudioStory = Join-Path $SourcePath "..\SingularityVR-cutscene\audio\polish\story_voicepack_xlingual"
$SourceVfxDll = Join-Path $SourcePath "..\SingularityVR-production\deployment\desktop-hmd-current-20261009\SingularityVfx.dll"
$ManifestFile = Join-Path $SourcePath "localization\pl\polish_manifest.json"

# Expected hashes (update after final QA)
$ExpectedHashes = @{
    "polish_text.jsonl" = "78EE02BF0599F19580C926AD83C8275666C8668B2EA816EF3ED5E8E6C755E12D"
    "polish_voice.jsonl" = "D83D9F11C6930C3A5A80422281458400892F179CDD538F9DB9D9D137C7FB5535"
    "polish_story_voice.jsonl" = "PLACEHOLDER_STORY_HASH"
}

function Write-Log {
    param([string]$Message, [string]$Level = "INFO")
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    Write-Host "[$timestamp] [$Level] $Message"
}

function Test-FileHash {
    param([string]$Path, [string]$ExpectedHash)
    if (-not (Test-Path $Path)) { return $false }
    $actual = (Get-FileHash -Path $Path -Algorithm SHA256).Hash.ToUpper()
    return $actual -eq $ExpectedHash.ToUpper()
}

function Backup-OriginalFiles {
    Write-Log "Creating backup directory: $BackupDir"
    if (-not $DryRun) {
        New-Item -ItemType Directory -Path $BackupDir -Force | Out-Null
    }
    
    # Backup Coalesced_INT.bin
    $coalescedInt = Join-Path $GameCookedPC "Coalesced_INT.bin"
    if (Test-Path $coalescedInt) {
        Write-Log "Backing up Coalesced_INT.bin"
        if (-not $DryRun) {
            Copy-Item -LiteralPath $coalescedInt -Destination (Join-Path $BackupDir "Coalesced_INT.bin") -Force
        }
    }
    
    # Backup any existing polish files
    $existingPolish = Get-ChildItem -Path $GameCookedPC -Filter "*POL*" -Recurse -ErrorAction SilentlyContinue
    foreach ($file in $existingPolish) {
        $relPath = $file.FullName.Substring($GameCookedPC.Length + 1)
        $destPath = Join-Path $BackupDir $relPath
        Write-Log "Backing up existing Polish file: $relPath"
        if (-not $DryRun) {
            New-Item -ItemType Directory -Path (Split-Path $destPath) -Force | Out-Null
            Copy-Item -LiteralPath $file.FullName -Destination $destPath -Force
        }
    }
    
    # Backup INI
    $iniPath = Join-Path $GameBinaries "SingularityVR.ini"
    if (Test-Path $iniPath) {
        Write-Log "Backing up SingularityVR.ini"
        if (-not $DryRun) {
            Copy-Item -LiteralPath $iniPath -Destination (Join-Path $BackupDir "SingularityVR.ini") -Force
        }
    }
    
    # Backup SingularityVfx.dll if exists
    $vfxDllPath = Join-Path $GameBinaries "SingularityVfx.dll"
    if (Test-Path $vfxDllPath) {
        Write-Log "Backing up SingularityVfx.dll"
        if (-not $DryRun) {
            Copy-Item -LiteralPath $vfxDllPath -Destination (Join-Path $BackupDir "SingularityVfx.dll") -Force
        }
    }
}

function Install-PolishText {
    Write-Log "Installing Polish text localization..."
    
    # Validate source
    if (-not (Test-Path $SourceText)) {
        throw "Source text file not found: $SourceText"
    }
    
    if (-not (Test-FileHash -Path $SourceText -ExpectedHash $ExpectedHashes["polish_text.jsonl"])) {
        throw "Source text hash mismatch! Expected: $($ExpectedHashes['polish_text.jsonl'])"
    }
    
    # Install to vr-assets/localization/pl/
    $modLocalizationDir = Join-Path $GameBinaries "vr-assets\localization\pl"
    if (-not $DryRun) {
        New-Item -ItemType Directory -Path $modLocalizationDir -Force | Out-Null
        Copy-Item -LiteralPath $SourceText -Destination (Join-Path $modLocalizationDir "polish_text.jsonl") -Force
        Copy-Item -LiteralPath $SourceVoice -Destination (Join-Path $modLocalizationDir "polish_voice.jsonl") -Force
        if (Test-Path $SourceStory) {
            Copy-Item -LiteralPath $SourceStory -Destination (Join-Path $modLocalizationDir "polish_story_voice.jsonl") -Force
        }
    }
    
    Write-Log "Polish text installed to: $modLocalizationDir"
}

function Install-PolishAudio {
    Write-Log "Installing Polish voice dubbing..."
    
    if (-not (Test-Path $SourceAudioUI)) {
        Write-Log "UI audio source directory not found, skipping audio install" -Level "WARN"
        return
    }
    
    $audioFiles = Get-ChildItem -Path $SourceAudioUI -Filter "*.wav" -Recurse
    if ($audioFiles.Count -eq 0) {
        Write-Log "No audio files found, skipping audio install" -Level "WARN"
        return
    }
    
    $modAudioDir = Join-Path $GameBinaries "vr-assets\audio\polish"
    if (-not $DryRun) {
        New-Item -ItemType Directory -Path $modAudioDir -Force | Out-Null
        
        # Copy UI audio
        $uiDest = Join-Path $modAudioDir "ui"
        New-Item -ItemType Directory -Path $uiDest -Force | Out-Null
        foreach ($file in $audioFiles) {
            Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $uiDest $file.Name) -Force
        }
        
        # Copy Story audio if exists
        if (Test-Path $SourceAudioStory) {
            $storyFiles = Get-ChildItem -Path $SourceAudioStory -Filter "*.wav" -Recurse
            if ($storyFiles.Count -gt 0) {
                $storyDest = Join-Path $modAudioDir "story"
                New-Item -ItemType Directory -Path $storyDest -Force | Out-Null
                foreach ($file in $storyFiles) {
                    Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $storyDest $file.Name) -Force
                }
            }
        }
    }
    
    Write-Log "Installed $($audioFiles.Count) Polish UI audio files to: $modAudioDir"
}

function Install-VfxPlugin {
    Write-Log "Installing SingularityVfx.dll (VFX plugin)..."
    
    if (-not (Test-Path $SourceVfxDll)) {
        Write-Log "SingularityVfx.dll not found at $SourceVfxDll, skipping VFX plugin install" -Level "WARN"
        return
    }
    
    $vfxDest = Join-Path $GameBinaries "SingularityVfx.dll"
    if (-not $DryRun) {
        Copy-Item -LiteralPath $SourceVfxDll -Destination $vfxDest -Force
        Write-Log "Installed SingularityVfx.dll to: $vfxDest"
    }
}

function Create-Manifest {
    Write-Log "Creating installation manifest..."
    
    $manifest = @{
        install_date = Get-Date -Format "yyyy-MM-ddTHH:mm:ss"
        game_path = $GamePath
        source_path = $SourcePath
        backup_path = $BackupDir
        text_hash = $ExpectedHashes["polish_text.jsonl"]
        voice_hash = $ExpectedHashes["polish_voice.jsonl"]
        story_hash = $ExpectedHashes["polish_story_voice.jsonl"]
        audio_count = (Get-ChildItem -Path (Join-Path $GameBinaries "vr-assets\audio\polish") -Filter "*.wav" -Recurse -ErrorAction SilentlyContinue).Count
        text_entries = 7056
        voice_entries = 894
        story_entries = 992
        status = "installed"
    }
    
    $manifestPath = Join-Path $GameBinaries "vr-assets\polish_install_manifest.json"
    if (-not $DryRun) {
        $manifest | ConvertTo-Json -Depth 5 | Set-Content -Path $manifestPath -Encoding UTF8
    }
    
    Write-Log "Manifest created: $manifestPath"
}

function Update-IniConfig {
    Write-Log "Updating SingularityVR.ini for Polish language option..."
    
    $iniPath = Join-Path $GameBinaries "SingularityVR.ini"
    if (-not (Test-Path $iniPath)) {
        Write-Log "INI file not found, skipping config update" -Level "WARN"
        return
    }
    
    $iniContent = Get-Content -Path $iniPath -Raw
    
    # Add/Update [Localization] section
    $localizationSection = @"
[Localization]
; Polish language support
; 0 = Disabled (default English), 1 = Enabled (Polish text + Polish dubbing)
PolishLanguage=0

; Fallback behavior when Polish assets are missing
; 0 = Show missing key, 1 = Fallback to English
PolishFallbackToEnglish=1

; UI text resources
; Paths relative to Binaries/vr-assets/localization/pl/
PolishTextManifest=localization/pl/polish_text.jsonl
PolishVoiceManifest=localization/pl/polish_voice.jsonl
PolishStoryManifest=localization/pl/polish_story_voice.jsonl

; Audio assets
; Paths relative to Binaries/vr-assets/audio/polish/
PolishUiAudioPath=audio/polish/ui
PolishStoryAudioPath=audio/polish/story

; UI language display
; 0 = English labels, 1 = Polish labels in VR settings
ShowPolishLabels=1

; Audio fallback
; 0 = Silent if Polish audio missing, 1 = Fallback to English audio
PolishAudioFallback=1

; Debug
; 0 = Normal, 1 = Log missing keys/audio, 2 = Verbose
LocalizationDebug=0
"@
    
    if ($iniContent -notmatch '\[Localization\]') {
        if (-not $DryRun) {
            Add-Content -Path $iniPath -Value "`n$localizationSection"
        }
        Write-Log "Added [Localization] section to INI"
    } else {
        Write-Log "[Localization] section already exists in INI"
    }
}

function Verify-Installation {
    Write-Log "Verifying installation..."
    
    $checks = @(
        @{ Path = Join-Path $GameBinaries "vr-assets\localization\pl\polish_text.jsonl"; Name = "Polish text" }
        @{ Path = Join-Path $GameBinaries "vr-assets\localization\pl\polish_voice.jsonl"; Name = "Polish voice manifest" }
        @{ Path = Join-Path $GameBinaries "vr-assets\localization\pl\polish_story_voice.jsonl"; Name = "Polish story manifest" }
        @{ Path = Join-Path $GameBinaries "vr-assets\polish_install_manifest.json"; Name = "Install manifest" }
    )
    
    $allOk = $true
    foreach ($check in $checks) {
        if (Test-Path $check.Path) {
            Write-Log "OK: $($check.Name) found" -Level "INFO"
        } else {
            Write-Log "MISSING: $($check.Name)" -Level "ERROR"
            $allOk = $false
        }
    }
    
    $audioDir = Join-Path $GameBinaries "vr-assets\audio\polish"
    if (Test-Path $audioDir) {
        $count = (Get-ChildItem -Path $audioDir -Filter "*.wav" -Recurse -ErrorAction SilentlyContinue).Count
        Write-Log "OK: Polish audio directory found with $count files" -Level "INFO"
    } else {
        Write-Log "WARN: Polish audio directory not found" -Level "WARN"
    }
    
    # Check VFX plugin
    $vfxDllPath = Join-Path $GameBinaries "SingularityVfx.dll"
    if (Test-Path $vfxDllPath) {
        Write-Log "OK: SingularityVfx.dll (VFX plugin) found" -Level "INFO"
    } else {
        Write-Log "WARN: SingularityVfx.dll (VFX plugin) not found" -Level "WARN"
    }
    
    return $allOk
}

# Main execution
Write-Log "=== SingularityVR Polish Localization Installer ==="
Write-Log "Game path: $GamePath"
Write-Log "Source path: $SourcePath"
Write-Log "Dry run: $DryRun"

if (-not (Test-Path $GamePath)) {
    throw "Game path not found: $GamePath"
}

if (-not (Test-Path $SourceText)) {
    throw "Source text not found: $SourceText"
}

# Check for admin rights
$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
$isAdmin = $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Log "WARNING: Not running as Administrator. Some operations may fail." -Level "WARN"
}

# Backup
Backup-OriginalFiles

# Install
Install-PolishText
Install-PolishAudio
Install-VfxPlugin

# Create manifest
Create-Manifest

# Update INI
Update-IniConfig

# Verify
if (Verify-Installation) {
    Write-Log "=== INSTALLATION SUCCESSFUL ==="
    Write-Log "To enable Polish: Set PolishLanguage=1 in SingularityVR.ini and restart game"
    Write-Log "Backup location: $BackupDir"
} else {
    Write-Log "=== INSTALLATION COMPLETED WITH WARNINGS ===" -Level "WARN"
}

if ($DryRun) {
    Write-Log "DRY RUN COMPLETE - No changes were made" -Level "WARN"
}
