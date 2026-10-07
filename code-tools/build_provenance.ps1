<#
.SYNOPSIS
    SingularityVR Build Provenance Verifier
    
.DESCRIPTION
    Verifies that a built DLL matches the expected repository/worktree/branch/commit.
    Prevents deployment of a DLL built from the wrong source.
    
    Reports:
    - Repository/worktree path
    - Branch
    - HEAD SHA
    - git status (clean/dirty)
    - Remote URL
    - Build DLL path
    - DLL timestamp
    - DLL SHA-256
    - DLL architecture (PE32/PE32+, x86/x64)
    
    Optionally compares against a deployed DLL.
    
.NOTES
    This tool is READ-ONLY. It does NOT deploy or modify any files.
    It does NOT build anything.
    
.PARAMETER BuildDll
    Path to the built d3d9.dll to verify. Default: attempts to find in common build output locations.
    
.PARAMETER DeployedDll
    Path to the currently deployed d3d9.dll to compare against. Default: C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\d3d9.dll
    
.PARAMETER Repo
    Path to the repository/worktree to check. Default: current script directory's git root.
    
.EXAMPLE
    .\build_provenance.ps1
    
.EXAMPLE
    .\build_provenance.ps1 -BuildDll "C:\build\d3d9.dll" -DeployedDll "C:\Games\Singularity\Binaries\d3d9.dll"
    
.EXAMPLE
    .\build_provenance.ps1 -Repo "C:\Users\Trilex\vr-mods\SingularityVR-production"
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory=$false)]
    [string]$BuildDll = "",
    
    [Parameter(Mandatory=$false)]
    [string]$DeployedDll = "C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\d3d9.dll",
    
    [Parameter(Mandatory=$false)]
    [string]$Repo = ""
)

$ErrorActionPreference = "Stop"

# Colors for output
$Green = [ConsoleColor]::Green
$Red = [ConsoleColor]::Red
$Yellow = [ConsoleColor]::Yellow
$Cyan = [ConsoleColor]::Cyan
$Gray = [ConsoleColor]::DarkGray
$White = [ConsoleColor]::White

function Write-Color([string]$msg, [ConsoleColor]$color = $White) {
    $orig = $Host.UI.RawUI.ForegroundColor
    $Host.UI.RawUI.ForegroundColor = $color
    Write-Host $msg
    $Host.UI.RawUI.ForegroundColor = $orig
}

function Write-Header([string]$msg) {
    Write-Color ("=" * 70) $Cyan
    Write-Color $msg $Cyan
    Write-Color ("=" * 70) $Cyan
}

function Write-Section([string]$msg) {
    Write-Color ("-" * 70) $Gray
    Write-Color $msg $Yellow
    Write-Color ("-" * 70) $Gray
}

function Write-KeyValue([string]$key, [string]$value, [ConsoleColor]$valueColor = $White) {
    $keyPad = $key.PadRight(30)
    Write-Host ("{0}: " -f $keyPad) -NoNewline
    Write-Color $value $valueColor
}

function Get-GitInfo($repoPath) {
    $result = @{}
    
    # Repository path
    $result.RepoPath = $repoPath
    
    # Current branch
    try {
        $branch = git -C $repoPath branch --show-current 2>$null
        $result.Branch = $branch.Trim()
    } catch {
        $result.Branch = "ERROR: $($_.Exception.Message)"
    }
    
    # HEAD SHA
    try {
        $sha = git -C $repoPath rev-parse HEAD 2>$null
        $result.HeadSha = $sha.Trim()
    } catch {
        $result.HeadSha = "ERROR: $($_.Exception.Message)"
    }
    
    # Short SHA
    try {
        $shortSha = git -C $repoPath rev-parse --short HEAD 2>$null
        $result.ShortSha = $shortSha.Trim()
    } catch {
        $result.ShortSha = "ERROR"
    }
    
    # Git status (porcelain)
    try {
        $statusLines = @(git -C $repoPath status --porcelain 2>$null)
        if ($statusLines -and ($statusLines | Where-Object { $_.Trim() } ).Count -gt 0) {
            $result.Status = "DIRTY"
            $result.StatusDetails = ($statusLines | ForEach-Object { "  $_" }) -join "`n"
        } else {
            $result.Status = "CLEAN"
            $result.StatusDetails = ""
        }
    } catch {
        $result.Status = "ERROR: $($_.Exception.Message)"
        $result.StatusDetails = ""
    }
    
    # Remote URL
    try {
        $remote = git -C $repoPath remote get-url origin 2>$null
        $result.Remote = $remote.Trim()
    } catch {
        $result.Remote = "ERROR: $($_.Exception.Message)"
    }
    
    # Log (last 5 commits)
    try {
        $log = git -C $repoPath log --oneline -5 2>$null
        $result.Log = $log.Trim()
    } catch {
        $result.Log = "ERROR: $($_.Exception.Message)"
    }
    
    return $result
}

function Get-DllInfo($dllPath) {
    $result = @{}
    
    if (-not (Test-Path $dllPath)) {
        $result.Exists = $false
        $result.Path = $dllPath
        $result.Error = "File not found"
        return $result
    }
    
    $result.Exists = $true
    $result.Path = $dllPath
    
    # File info
    $fileInfo = Get-Item $dllPath
    $result.Timestamp = $fileInfo.LastWriteTime
    $result.Size = $fileInfo.Length
    
    # SHA-256
    try {
        $sha256 = Get-FileHash -Path $dllPath -Algorithm SHA256
        $result.Sha256 = $sha256.Hash.ToLower()
    } catch {
        $result.Sha256 = "ERROR: $($_.Exception.Message)"
    }
    
    # PE header analysis
    try {
        $bytes = [System.IO.File]::ReadAllBytes($dllPath)
        
        # Check DOS header
        if ($bytes.Length -lt 64 -or $bytes[0] -ne 0x4D -or $bytes[1] -ne 0x5A) {
            $result.Architecture = "INVALID: Not a valid PE file (no MZ header)"
            return $result
        }
        
        # PE header offset at 0x3C
        $peOffset = [BitConverter]::ToUInt32($bytes, 0x3C)
        if ($peOffset + 4 -gt $bytes.Length) {
            $result.Architecture = "INVALID: PE header offset out of bounds"
            return $result
        }
        
        # Check PE signature
        if ($bytes[$peOffset] -ne 0x50 -or $bytes[$peOffset+1] -ne 0x45 -or $bytes[$peOffset+2] -ne 0x00 -or $bytes[$peOffset+3] -ne 0x00) {
            $result.Architecture = "INVALID: No PE signature at offset 0x{0:X}" -f $peOffset
            return $result
        }
        
        # COFF header (Machine field at offset 4 from PE sig)
        $coffOffset = $peOffset + 4
        $machine = [BitConverter]::ToUInt16($bytes, $coffOffset)
        
        switch ($machine) {
            0x014c { $result.Machine = "x86 (IMAGE_FILE_MACHINE_I386)" }
            0x8664 { $result.Machine = "x64 (IMAGE_FILE_MACHINE_AMD64)" }
            0x0200 { $result.Machine = "IA64 (IMAGE_FILE_MACHINE_IA64)" }
            0x01c0 { $result.Machine = "ARM (IMAGE_FILE_MACHINE_ARM)" }
            0xaa64 { $result.Machine = "ARM64 (IMAGE_FILE_MACHINE_ARM64)" }
            default { $result.Machine = "Unknown (0x{0:X4})" -f $machine }
        }
        
        # Optional header magic (PE32 vs PE32+)
        $optHeaderOffset = $coffOffset + 20
        if ($optHeaderOffset + 2 -le $bytes.Length) {
            $magic = [BitConverter]::ToUInt16($bytes, $optHeaderOffset)
            switch ($magic) {
                0x010b { $result.PeFormat = "PE32 (32-bit)" }
                0x020b { $result.PeFormat = "PE32+ (64-bit)" }
                0x0107 { $result.PeFormat = "ROM image" }
                default { $result.PeFormat = "Unknown magic (0x{0:X4})" -f $magic }
            }
        }
        
        # Subsystem
        $subsystemOffset = $optHeaderOffset + 68
        if ($subsystemOffset + 2 -le $bytes.Length) {
            $subsystem = [BitConverter]::ToUInt16($bytes, $subsystemOffset)
            switch ($subsystem) {
                1 { $result.Subsystem = "NATIVE" }
                2 { $result.Subsystem = "WINDOWS_GUI" }
                3 { $result.Subsystem = "WINDOWS_CUI" }
                7 { $result.Subsystem = "POSIX_CUI" }
                9 { $result.Subsystem = "WINDOWS_CE_GUI" }
                10 { $result.Subsystem = "EFI_APPLICATION" }
                11 { $result.Subsystem = "EFI_BOOT_SERVICE_DRIVER" }
                12 { $result.Subsystem = "EFI_RUNTIME_DRIVER" }
                13 { $result.Subsystem = "EFI_ROM" }
                14 { $result.Subsystem = "XBOX" }
                16 { $result.Subsystem = "WINDOWS_BOOT_APPLICATION" }
                default { $result.Subsystem = "Unknown (0x{0:X4})" -f $subsystem }
            }
        }
        
        # DLL characteristics
        $dllCharOffset = $subsystemOffset + 2
        if ($dllCharOffset + 2 -le $bytes.Length) {
            $dllChars = [BitConverter]::ToUInt16($bytes, $dllCharOffset)
            $flags = @()
            if ($dllChars -band 0x0001) { $flags += "HIGH_ENTROPY_VA" }
            if ($dllChars -band 0x0002) { $flags += "DYNAMIC_BASE (ASLR)" }
            if ($dllChars -band 0x0004) { $flags += "FORCE_INTEGRITY" }
            if ($dllChars -band 0x0008) { $flags += "NX_COMPAT (DEP)" }
            if ($dllChars -band 0x0020) { $flags += "NO_ISOLATION" }
            if ($dllChars -band 0x0040) { $flags += "NO_SEH" }
            if ($dllChars -band 0x0080) { $flags += "NO_BIND" }
            if ($dllChars -band 0x0100) { $flags += "APPCONTAINER" }
            if ($dllChars -band 0x0200) { $flags += "WDM_DRIVER" }
            if ($dllChars -band 0x0400) { $flags += "GUARD_CF" }
            if ($dllChars -band 0x0800) { $flags += "TERMINAL_SERVER_AWARE" }
            $result.DllCharacteristics = $flags -join ", "
        }
        
        $result.Architecture = "{0}, {1}" -f $result.Machine, $result.PeFormat
        
    } catch {
        $result.Architecture = "ERROR: $($_.Exception.Message)"
    }
    
    return $result
}

function Compare-Dlls($buildInfo, $deployedInfo) {
    if (-not $buildInfo.Exists -or -not $deployedInfo.Exists) {
        return $null
    }
    
    $match = $buildInfo.Sha256 -eq $deployedInfo.Sha256
    return $match
}

# ============================================================
# MAIN
# ============================================================

Write-Header "SINGULARITYVR BUILD PROVENANCE VERIFIER"
Write-Host "Tool: code-tools\build_provenance.ps1"
Write-Host "Mode: READ-ONLY (no deployment, no modification)"
Write-Host ""

# Determine repo path
if (-not $Repo) {
    # Try to find git root from script location
    $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
    $gitRoot = git -C $scriptDir rev-parse --show-toplevel 2>$null
    if ($gitRoot) {
        $Repo = $gitRoot.Trim()
    } else {
        $Repo = (Get-Location).Path
    }
}

# Resolve BuildDll if not provided
if (-not $BuildDll) {
    # Common build output locations
    $candidates = @(
        "$Repo\build\d3d9.dll",
        "$Repo\spikes\view_matrix\build\d3d9.dll",
        "$Repo\spikes\view_matrix\d3d9.dll",
        "$Repo\Release\d3d9.dll",
        "$Repo\x64\Release\d3d9.dll",
        "$Repo\x86\Release\d3d9.dll",
        "C:\Users\Trilex\vr-mods\SingularityVR-production\spikes\view_matrix\d3d9.dll"
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) {
            $BuildDll = $c
            break
        }
    }
    if (-not $BuildDll) {
        Write-Color "WARNING: Could not auto-detect build DLL. Use -BuildDll to specify." $Yellow
    }
}

# ============================================================
# REPOSITORY INFO
# ============================================================
Write-Section "REPOSITORY / WORKTREE"
$gitInfo = Get-GitInfo $Repo
Write-KeyValue "Path" $gitInfo.RepoPath
Write-KeyValue "Branch" $gitInfo.Branch
Write-KeyValue "HEAD SHA" $gitInfo.HeadSha
Write-KeyValue "Short SHA" $gitInfo.ShortSha
if ($gitInfo.Status -eq "CLEAN") { $statusColor = $Green } else { $statusColor = $Red }
Write-KeyValue "Status" $gitInfo.Status $statusColor
if ($gitInfo.StatusDetails) {
    Write-Host "Details:`n$($gitInfo.StatusDetails)"
}
Write-KeyValue "Remote" $gitInfo.Remote
Write-Host ""
Write-Color "Recent commits:" $Gray
$gitInfo.Log.Split("`n") | ForEach-Object { Write-Host "  $_" }

# ============================================================
# BUILD DLL INFO
# ============================================================
Write-Section "BUILD DLL"
$buildInfo = Get-DllInfo $BuildDll
if ($buildInfo.Exists) {
    Write-KeyValue "Path" $buildInfo.Path
    Write-KeyValue "Timestamp" $buildInfo.Timestamp.ToString("yyyy-MM-dd HH:mm:ss")
    Write-KeyValue "Size" ("{0} bytes" -f $buildInfo.Size)
    Write-KeyValue "SHA-256" $buildInfo.Sha256
    Write-KeyValue "Architecture" $buildInfo.Architecture
    if ($buildInfo.Machine) { Write-KeyValue "Machine" $buildInfo.Machine }
    if ($buildInfo.PeFormat) { Write-KeyValue "PE Format" $buildInfo.PeFormat }
    if ($buildInfo.Subsystem) { Write-KeyValue "Subsystem" $buildInfo.Subsystem }
    if ($buildInfo.DllCharacteristics) { Write-KeyValue "DLL Characteristics" $buildInfo.DllCharacteristics }
} else {
    Write-KeyValue "Path" $buildInfo.Path
    Write-KeyValue "ERROR" $buildInfo.Error $Red
}

# ============================================================
# DEPLOYED DLL INFO
# ============================================================
Write-Section "DEPLOYED DLL"
$deployedInfo = Get-DllInfo $DeployedDll
if ($deployedInfo.Exists) {
    Write-KeyValue "Path" $deployedInfo.Path
    Write-KeyValue "Timestamp" $deployedInfo.Timestamp.ToString("yyyy-MM-dd HH:mm:ss")
    Write-KeyValue "Size" ("{0} bytes" -f $deployedInfo.Size)
    Write-KeyValue "SHA-256" $deployedInfo.Sha256
    Write-KeyValue "Architecture" $deployedInfo.Architecture
    if ($deployedInfo.Machine) { Write-KeyValue "Machine" $deployedInfo.Machine }
    if ($deployedInfo.PeFormat) { Write-KeyValue "PE Format" $deployedInfo.PeFormat }
    if ($deployedInfo.Subsystem) { Write-KeyValue "Subsystem" $deployedInfo.Subsystem }
    if ($deployedInfo.DllCharacteristics) { Write-KeyValue "DLL Characteristics" $deployedInfo.DllCharacteristics }
} else {
    Write-KeyValue "Path" $deployedInfo.Path
    Write-KeyValue "ERROR" $deployedInfo.Error $Yellow
}

# ============================================================
# COMPARISON
# ============================================================
Write-Section "COMPARISON"
if ($buildInfo.Exists -and $deployedInfo.Exists) {
    $match = Compare-Dlls $buildInfo $deployedInfo
    if ($match) {
        Write-Color "BUILD == DEPLOYED: YES" $Green
    } else {
        Write-Color "BUILD == DEPLOYED: NO" $Red
    }
    
    Write-Host ""
    Write-Color "Build SHA-256:    $($buildInfo.Sha256)" $Gray
    Write-Color "Deployed SHA-256: $($deployedInfo.Sha256)" $Gray
    
    if (-not $match) {
        Write-Host ""
        Write-Color "MISMATCH DETECTED - Do not deploy this build!" $Red
    }
} elseif ($buildInfo.Exists -and -not $deployedInfo.Exists) {
    Write-Color "BUILD == DEPLOYED: N/A (deployed DLL not found)" $Yellow
} elseif (-not $buildInfo.Exists -and $deployedInfo.Exists) {
    Write-Color "BUILD == DEPLOYED: N/A (build DLL not found)" $Yellow
} else {
    Write-Color "BUILD == DEPLOYED: N/A (neither DLL found)" $Red
}

Write-Host ""
Write-Color ("=" * 70) $Cyan
Write-Color "VERIFICATION COMPLETE" $Cyan
Write-Color ("=" * 70) $Cyan

# Exit code: 0 = match, 1 = mismatch, 2 = error
if ($buildInfo.Exists -and $deployedInfo.Exists) {
    if ((Compare-Dlls $buildInfo $deployedInfo)) { exit 0 } else { exit 1 }
} elseif (-not $buildInfo.Exists -and -not $deployedInfo.Exists) {
    exit 2
} else {
    exit 2
}