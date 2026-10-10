# Resolves OpenXR SDK layout for x86 cl.exe builds (Khronos prebuilt or CMake install prefix).

function Get-OpenXrSdkPaths {
    param(
        [Parameter(Mandatory = $true)][string] $RepoRoot
    )

    $candidates = [System.Collections.Generic.List[string]]::new()
    if ($env:OPENXR_SDK) { $candidates.Add($env:OPENXR_SDK.Trim()) }

    $siblingDeps = Join-Path $RepoRoot '..\dependencies\openxr-local'
    $candidates.Add((Resolve-Path -LiteralPath $siblingDeps -ErrorAction SilentlyContinue))
    $candidates.Add((Join-Path $RepoRoot 'third_party\openxr-sdk'))
    $candidates.Add((Join-Path $env:USERPROFILE 'openxr-sdk'))
    $candidates.Add('R:\SingularityVR-Dev\tools\openxr-sdk')

    $tried = @()
    foreach ($base in $candidates) {
        if (-not $base) { continue }
        $tried += $base
        if (-not (Test-Path -LiteralPath $base)) { continue }

        $inc = Join-Path $base 'include'
        if (-not (Test-Path -LiteralPath $inc)) { continue }

        $khLib = Join-Path $base 'native\Win32\release\lib'
        $khBin = Join-Path $base 'native\Win32\release\bin'
        if ((Test-Path -LiteralPath (Join-Path $khLib 'openxr_loader.lib')) -and
            (Test-Path -LiteralPath (Join-Path $khBin 'openxr_loader.dll'))) {
            return [pscustomobject]@{
                Sdk    = $base
                Inc    = $inc
                LibDir = $khLib
                BinDir = $khBin
            }
        }

        $cmLib = Join-Path $base 'lib'
        $cmBin = Join-Path $base 'bin'
        if ((Test-Path -LiteralPath (Join-Path $cmLib 'openxr_loader.lib')) -and
            (Test-Path -LiteralPath (Join-Path $cmBin 'openxr_loader.dll'))) {
            return [pscustomobject]@{
                Sdk    = $base
                Inc    = $inc
                LibDir = $cmLib
                BinDir = $cmBin
            }
        }
    }

    $msg = @(
        'OpenXR SDK not found (need include\, openxr_loader.lib, openxr_loader.dll).'
        'Set OPENXR_SDK to a Khronos Win32 SDK or a CMake install prefix.'
        "Checked: $($tried -join '; ')"
    ) -join ' '
    throw $msg
}

function Get-SingularityGameBinaries {
    param(
        [string] $ExplicitPath
    )

    if ($ExplicitPath) { return $ExplicitPath }
    if ($env:SINGULARITY_BINARIES) { return $env:SINGULARITY_BINARIES.Trim() }
    if ($env:SINGULARITY_DEV_GAME) {
        return (Join-Path $env:SINGULARITY_DEV_GAME.Trim() 'Binaries')
    }

    foreach ($p in @(
            'R:\SingularityVR-Dev\Singularity\Binaries',
            'C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries'
        )) {
        if (Test-Path -LiteralPath $p) { return $p }
    }

    return $null
}
