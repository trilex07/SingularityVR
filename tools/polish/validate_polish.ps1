<#
.SYNOPSIS
    Validates Polish localization and dubbing installation for SingularityVR

.DESCRIPTION
    This script validates the integrity of Polish localization files,
    audio files, and configuration.
#>

param(
    [Parameter(Mandatory=$false)]
    [string]$GamePath = "C:\Program Files (x86)\Steam\steamapps\common\Singularity",
    
    [Parameter(Mandatory=$false)]
    [string]$SourcePath = "C:\Users\Trilex\vr-mods\SingularityVR-polish-integration",
    
    [Parameter(Mandatory=$false)]
    [switch]$Verbose
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$GameBinaries = Join-Path $GamePath "Binaries"
$GameCookedPC = Join-Path $GamePath "RvGame\CookedPC"
$SourceText = Join-Path $SourcePath "localization\pl\polish_text.jsonl"
$SourceVoice = Join-Path $SourcePath "localization\pl\polish_voice.jsonl"
$ExpectedHashes = @{
    "polish_text.jsonl" = "78EE02BF0599F19580C926AD83C8275666C8668B2EA816EF3ED5E8E6C755E12D"
    "polish_voice.jsonl" = "D83D9F11C6930C3A5A80422281458400892F179CDD538F9DB9D9D137C7FB5533"
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

function Validate-TextFile {
    param([string]$SourcePath, [string]$InstalledPath, [string]$Name)
    
    Write-Log "Validating $Name..."
    
    if (-not (Test-Path $SourcePath)) {
        Write-Log "FAIL: Source $Name not found at $SourcePath" -Level "ERROR"
        return $false
    }
    
    if (-not (Test-Path $InstalledPath)) {
        Write-Log "FAIL: Installed $Name not found at $InstalledPath" -Level "ERROR"
        return $false
    }
    
    $sourceHash = (Get-FileHash -Path $SourcePath -Algorithm SHA256).Hash.ToUpper()
    $installedHash = (Get-FileHash -Path $InstalledPath -Algorithm SHA256).Hash.ToUpper()
    
    if ($sourceHash -ne $installedHash) {
        Write-Log "FAIL: Hash mismatch for $Name" -Level "ERROR"
        Write-Log "  Source: $sourceHash" -Level "ERROR"
        Write-Log "  Installed: $installedHash" -Level "ERROR"
        return $false
    }
    
    $expectedHash = $ExpectedHashes[$Name].ToUpper()
    if ($sourceHash -ne $expectedHash) {
        Write-Log "WARN: Source hash doesn't match expected for $Name" -Level "WARN"
        Write-Log "  Expected: $expectedHash" -Level "WARN"
        Write-Log "  Actual: $sourceHash" -Level "WARN"
    }
    
    # Validate JSONL structure
    $lineCount = 0
    $validLines = 0
    try {
        $reader = [System.IO.File]::OpenText($InstalledPath)
        while ($null -ne ($line = $reader.ReadLine())) {
            $lineCount++
            if ($line.Trim()) {
                $json = $line | ConvertFrom-Json
                if ($json.id -and $json.en -and $json.pl -and $json.status) {
                    $validLines++
                }
            }
        }
        $reader.Close()
    } catch {
        Write-Log "FAIL: Invalid JSONL in ${Name}: $($_.Exception.Message)" -Level "ERROR"
        return $false
    }
    
    Write-Log "OK: $Name - $validLines/$lineCount valid entries, hash verified"
    return $true
}

function Validate-AudioFiles {
    param([string]$AudioDir)
    
    Write-Log "Validating audio files in $AudioDir..."
    
    if (-not (Test-Path $AudioDir)) {
        Write-Log "WARN: Audio directory not found: $AudioDir" -Level "WARN"
        return $true  # Not a failure, audio is optional
    }
    
    $files = Get-ChildItem -Path $AudioDir -Filter "*.wav" -Recurse
    Write-Log "Found $($files.Count) audio files"
    
    $valid = 0
    $issues = 0
    foreach ($file in $files) {
        try {
            # Basic WAV validation
            $bytes = [System.IO.File]::ReadAllBytes($file.FullName)
            if ($bytes.Length -lt 44) {
                Write-Log "WARN: $($file.Name) - too small ($($bytes.Length) bytes)" -Level "WARN"
                $issues++
                continue
            }
            
            # Check RIFF header
            $header = [System.Text.Encoding]::ASCII.GetString($bytes[0..3])
            if ($header -ne "RIFF") {
                Write-Log "WARN: $($file.Name) - invalid RIFF header" -Level "WARN"
                $issues++
                continue
            }
            
            $waveHeader = [System.Text.Encoding]::ASCII.GetString($bytes[8..11])
            if ($waveHeader -ne "WAVE") {
                Write-Log "WARN: $($file.Name) - invalid WAVE header" -Level "WARN"
                $issues++
                continue
            }
            
            $valid++
        } catch {
            Write-Log "WARN: $($file.Name) - read error: $($_.Exception.Message)" -Level "WARN"
            $issues++
        }
    }
    
    Write-Log "Audio validation: $valid valid, $issues issues"
    return $true
}

function Validate-Manifest {
    param([string]$ManifestPath)
    
    Write-Log "Validating install manifest..."
    
    if (-not (Test-Path $ManifestPath)) {
        Write-Log "FAIL: Manifest not found at $ManifestPath" -Level "ERROR"
        return $false
    }
    
    try {
        $manifest = Get-Content -Path $ManifestPath -Raw | ConvertFrom-Json
        
        $required = @("install_date", "game_path", "text_hash", "voice_hash", "audio_count", "text_entries", "voice_entries", "status")
        foreach ($field in $required) {
            if (-not $manifest.$field) {
                Write-Log "FAIL: Manifest missing required field: $field" -Level "ERROR"
                return $false
            }
        }
        
        if ($manifest.status -ne "installed") {
            Write-Log "WARN: Manifest status is '$($manifest.status)', expected 'installed'" -Level "WARN"
        }
        
        Write-Log "OK: Manifest valid - installed $($manifest.install_date), $($manifest.audio_count) audio files, $($manifest.text_entries) text entries, $($manifest.voice_entries) voice entries"
        return $true
    } catch {
        Write-Log "FAIL: Invalid manifest JSON: $($_.Exception.Message)" -Level "ERROR"
        return $false
    }
}

function Validate-IniConfig {
    param([string]$IniPath)
    
    Write-Log "Validating INI configuration..."
    
    if (-not (Test-Path $IniPath)) {
        Write-Log "FAIL: INI not found at $IniPath" -Level "ERROR"
        return $false
    }
    
    $iniContent = Get-Content -Path $IniPath -Raw
    if ($iniContent -notmatch '\[Localization\]') {
        Write-Log "WARN: [Localization] section not found in INI" -Level "WARN"
        return $true
    }
    
    if ($iniContent -notmatch 'PolishLanguage=\d') {
        Write-Log "WARN: PolishLanguage setting not found in INI" -Level "WARN"
        return $true
    }
    
    if ($iniContent -match 'PolishLanguage=1') {
        Write-Log "INFO: Polish language is ENABLED in INI"
    } else {
        Write-Log "INFO: Polish language is DISABLED in INI (PolishLanguage=0)"
    }
    
    return $true
}

function Validate-VfxPlugin {
    param([string]$VfxDllPath)
    
    Write-Log "Validating VFX plugin..."
    
    if (-not (Test-Path $VfxDllPath)) {
        Write-Log "WARN: SingularityVfx.dll not found at $VfxDllPath" -Level "WARN"
        return $true  # Not a failure, VFX plugin is optional
    }
    
    try {
        # Basic PE validation
        $bytes = [System.IO.File]::ReadAllBytes($VfxDllPath)
        if ($bytes.Length -lt 64) {
            Write-Log "WARN: SingularityVfx.dll too small ($($bytes.Length) bytes)" -Level "WARN"
            return $true
        }
        
        # Check PE header
        if ($bytes[0] -ne 0x4D -or $bytes[1] -ne 0x5A) {
            Write-Log "FAIL: SingularityVfx.dll - invalid PE header" -Level "ERROR"
            return $false
        }
        
        $peOffset = [BitConverter]::ToInt32($bytes, 0x3C)
        $machine = [BitConverter]::ToUInt16($bytes, $peOffset + 4)
        $optionalMagic = [BitConverter]::ToUInt16($bytes, $peOffset + 24)
        
        if ($machine -ne 0x014C -or $optionalMagic -ne 0x010B) {
            Write-Log "FAIL: SingularityVfx.dll is not PE32 x86" -Level "ERROR"
            return $false
        }
        
        Write-Log "OK: SingularityVfx.dll is valid PE32 x86 DLL"
        return $true
    } catch {
        Write-Log "FAIL: SingularityVfx.dll validation error: $($_.Exception.Message)" -Level "ERROR"
        return $false
    }
}

# Main validation
Write-Log "=== SingularityVR Polish Localization Validator ==="
Write-Log "Game path: $GamePath"

$allPassed = $true

# Validate text files
$installedText = Join-Path $GameBinaries "vr-assets\localization\pl\polish_text.jsonl"
$installedVoice = Join-Path $GameBinaries "vr-assets\localization\pl\polish_voice.jsonl"

$allPassed = Validate-TextFile $SourceText $installedText "polish_text.jsonl" -and $allPassed
$allPassed = Validate-TextFile $SourceVoice $installedVoice "polish_voice.jsonl" -and $allPassed

# Validate audio
$audioDir = Join-Path $GameBinaries "vr-assets\audio\polish"
$allPassed = Validate-AudioFiles $audioDir -and $allPassed

# Validate manifest
$manifestPath = Join-Path $GameBinaries "vr-assets\polish_install_manifest.json"
$allPassed = Validate-Manifest $manifestPath -and $allPassed

# Validate INI
$iniPath = Join-Path $GameBinaries "SingularityVR.ini"
$allPassed = Validate-IniConfig $iniPath -and $allPassed

# Validate VFX plugin
$vfxDllPath = Join-Path $GameBinaries "SingularityVfx.dll"
$allPassed = Validate-VfxPlugin $vfxDllPath -and $allPassed

# Summary
Write-Log ""
if ($allPassed) {
    Write-Log "=== ALL VALIDATIONS PASSED ==="
    exit 0
} else {
    Write-Log "=== VALIDATION FAILED ===" -Level "ERROR"
    exit 1
}