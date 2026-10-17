[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$AcceptedProfilePath,
    [Parameter(Mandatory = $true)]
    [string]$ArtifactDir,
    [string]$DropVSHash,
    [string]$DropPSHash,
    [int]$DropVSBytes = 0,
    [int]$DropPSBytes = 0,
    [switch]$NoProbe,
    [string]$SteamRoot = 'C:\Program Files (x86)\Steam'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$artifactPath = [System.IO.Path]::GetFullPath($ArtifactDir)
$repoPrefix = $repoRoot.TrimEnd('\') + '\'
if (-not $artifactPath.StartsWith($repoPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "ArtifactDir must be inside the MAIN worktree: $repoRoot"
}
if (Test-Path -LiteralPath $artifactPath) {
    throw "Refusing to overwrite existing artifact directory: $artifactPath"
}

$profileFullPath = [System.IO.Path]::GetFullPath($AcceptedProfilePath)
if (-not (Test-Path -LiteralPath $profileFullPath -PathType Leaf)) {
    throw "Accepted profile not found: $profileFullPath"
}
$profile = Get-Content -LiteralPath $profileFullPath -Raw | ConvertFrom-Json
if ($profile.schema -ne 'singularityvr-accepted-profile-v1' -or $profile.approved -ne $true) {
    throw 'Accepted profile is not an approved SingularityVR profile.'
}
$profileSha = (Get-FileHash -LiteralPath $profileFullPath -Algorithm SHA256).Hash.ToUpperInvariant()

$gameBin = Join-Path $SteamRoot 'steamapps\common\Singularity\Binaries'
$installedIni = Join-Path $gameBin 'SingularityVR.ini'
$installedDll = Join-Path $gameBin 'd3d9.dll'
$installedDinput = Join-Path $gameBin 'dinput8.dll'
$candidateDll = Join-Path $repoRoot 'spikes\view_matrix\d3d9.dll'
foreach ($path in @($installedIni, $installedDll, $installedDinput, $candidateDll)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Required file is missing: $path" }
}

function Get-ProfileFileHash([string]$RelativePath) {
    $entry = @($profile.files | Where-Object { $_.path -ceq $RelativePath })
    if ($entry.Count -ne 1) { throw "Profile must pin exactly one $RelativePath entry." }
    return ([string]$entry[0].sha256).ToUpperInvariant()
}

function Get-ProfileIniValue([string]$Key) {
    $property = $profile.ini.PSObject.Properties | Where-Object { $_.Name -ceq $Key }
    if (@($property).Count -ne 1) { throw "Profile must pin exactly one INI key $Key." }
    return [string]$property.Value
}

function Get-IniSectionValues([string]$Text) {
    $result = @{}
    $section = ''
    foreach ($line in ($Text -split "`r?`n")) {
        if ($line -match '^\s*\[([^\]]+)\]\s*$') {
            $section = $Matches[1]
            continue
        }
        if ($line -match '^\s*([^;#][^=]*?)\s*=\s*(.*?)\s*$') {
            $key = $section + '/' + $Matches[1].Trim()
            if ($result.ContainsKey($key)) { throw "Duplicate INI key: $key" }
            $result[$key] = $Matches[2]
        }
    }
    return $result
}

function Replace-UniqueAscii([byte[]]$Bytes, [string]$Before, [string]$After) {
    $old = [System.Text.Encoding]::ASCII.GetBytes($Before)
    $new = [System.Text.Encoding]::ASCII.GetBytes($After)
    $positions = [System.Collections.Generic.List[int]]::new()
    for ($i = 0; $i -le $Bytes.Length - $old.Length; $i++) {
        $match = $true
        for ($j = 0; $j -lt $old.Length; $j++) {
            if ($Bytes[$i + $j] -ne $old[$j]) { $match = $false; break }
        }
        if ($match) { $positions.Add($i) }
    }
    if ($positions.Count -ne 1) { throw "Expected exactly one byte sequence '$Before'; found $($positions.Count)." }
    $copy = [byte[]]::new($Bytes.Length - $old.Length + $new.Length)
    [Array]::Copy($Bytes, 0, $copy, 0, $positions[0])
    [Array]::Copy($new, 0, $copy, $positions[0], $new.Length)
    $tailStart = $positions[0] + $old.Length
    $tailCount = $Bytes.Length - $tailStart
    [Array]::Copy($Bytes, $tailStart, $copy, $positions[0] + $new.Length, $tailCount)
    return $copy
}

$expectedDll = Get-ProfileFileHash 'Binaries/d3d9.dll'
$expectedDinput = Get-ProfileFileHash 'Binaries/dinput8.dll'
$actualDll = (Get-FileHash -LiteralPath $installedDll -Algorithm SHA256).Hash.ToUpperInvariant()
$actualDinput = (Get-FileHash -LiteralPath $installedDinput -Algorithm SHA256).Hash.ToUpperInvariant()
if ($actualDll -ne $expectedDll) { throw "Installed d3d9.dll differs from accepted profile: $actualDll" }
if ($actualDinput -ne $expectedDinput) { throw "Installed dinput8.dll differs from accepted profile: $actualDinput" }

$iniBytes = [System.IO.File]::ReadAllBytes($installedIni)
$iniText = [System.Text.UTF8Encoding]::new($false, $true).GetString($iniBytes)
$iniValues = Get-IniSectionValues $iniText
$probeKey = 'Diagnostic/NativeMuzzleProbe'
$modeKey = 'Diagnostic/NativeMuzzleStereoMode'
$probeBefore = Get-ProfileIniValue $probeKey
$modeBefore = Get-ProfileIniValue $modeKey
if (-not $iniValues.ContainsKey($probeKey) -or $iniValues[$probeKey] -ne $probeBefore) {
    throw "Installed INI does not match accepted profile at $probeKey."
}
if (-not $iniValues.ContainsKey($modeKey) -or $iniValues[$modeKey] -ne $modeBefore) {
    throw "Installed INI does not match accepted profile at $modeKey."
}
if ($probeBefore -ne '0' -or $modeBefore -ne '1') {
    throw "Expected accepted baseline $probeKey=0 and $modeKey=1; got $probeBefore / $modeBefore."
}

$dropRequested = $DropVSHash -or $DropPSHash -or $DropVSBytes -or $DropPSBytes
if ($NoProbe -and -not $dropRequested) {
    throw '-NoProbe is only valid for an exact-pair visual discriminator; baseline capture requires the bounded probe.'
}
if ($dropRequested) {
    if (-not $DropVSHash -or -not $DropPSHash -or $DropVSBytes -le 0 -or $DropPSBytes -le 0) {
        throw 'A mode-1 drop needs all four values: VS/PS hashes and byte lengths.'
    }
    if ($DropVSHash -notmatch '^[0-9A-Fa-f]{16}$' -or $DropPSHash -notmatch '^[0-9A-Fa-f]{16}$') {
        throw 'Drop shader hashes must each be exactly 16 hexadecimal characters.'
    }
    foreach ($size in @($DropVSBytes, $DropPSBytes)) {
        if ($size -lt 8 -or $size -gt 32768 -or ($size % 4) -ne 0) {
            throw "Invalid drop shader byte length: $size."
        }
    }
    if ($modeBefore -ne '1') { throw 'A selected-pair drop requires the accepted baseline mode=1.' }
}

$probeAfter = if ($NoProbe) { '0' } else { '1' }
$probeReason = if ($NoProbe) {
    'Keep the bounded trace/capture probe OFF so its GPU readback cannot contaminate the visual/performance discriminator.'
} else {
    'Enable the bounded idle plus three-shot driver traces.'
}
$candidateIniBytes = Replace-UniqueAscii $iniBytes "NativeMuzzleProbe=$probeBefore" "NativeMuzzleProbe=$probeAfter"
$iniChanges = [System.Collections.Generic.List[object]]::new()
if ($probeAfter -ne $probeBefore) {
    $iniChanges.Add([ordered]@{ key = $probeKey; before = $probeBefore; after = $probeAfter; reason = $probeReason })
}
if ($dropRequested) {
    $targets = [ordered]@{
        'Diagnostic/NativeMuzzleVSHash' = $DropVSHash.ToUpperInvariant()
        'Diagnostic/NativeMuzzleVSBytes' = [string]$DropVSBytes
        'Diagnostic/NativeMuzzlePSHash' = $DropPSHash.ToUpperInvariant()
        'Diagnostic/NativeMuzzlePSBytes' = [string]$DropPSBytes
    }
    foreach ($key in $targets.Keys) {
        $name = $key.Substring($key.IndexOf('/') + 1)
        $before = Get-ProfileIniValue $key
        $after = [string]$targets[$key]
        $candidateIniBytes = Replace-UniqueAscii $candidateIniBytes "$name=$before" "$name=$after"
        if ($before -ne $after) {
            $iniChanges.Add([ordered]@{ key = $key; before = $before; after = $after; reason = 'Exact mode-1 pair chosen for the next bounded MAIN discriminator; this is not a renderer fix.' })
        }
    }
    $modeAfter = $modeBefore
    if ($NoProbe) {
        $planReason = "MAIN L3 visual-only exact-pair drop discriminator for measured pair $($DropVSHash.ToUpperInvariant()):$($DropPSHash.ToUpperInvariant()); bounded probe OFF to avoid capture hitches; no stereo alias repair."
    } else {
        $planReason = "MAIN L3 exact-pair drop discriminator for measured pair $($DropVSHash.ToUpperInvariant()):$($DropPSHash.ToUpperInvariant()); keep the bounded probe ON; no stereo alias repair."
    }
} else {
    $modeAfter = '0'
    $candidateIniBytes = Replace-UniqueAscii $candidateIniBytes "NativeMuzzleStereoMode=$modeBefore" "NativeMuzzleStereoMode=$modeAfter"
    $iniChanges.Add([ordered]@{ key = $modeKey; before = $modeBefore; after = $modeAfter; reason = 'Keep the renderer unchanged for the native-only baseline capture.' })
    $planReason = 'MAIN L3 native muzzle capture: preserve accepted profile; set only NativeMuzzleProbe=1 and NativeMuzzleStereoMode=0 for native-only baseline traces.'
}
if (-not $dropRequested -and $candidateIniBytes.Length -ne $iniBytes.Length) {
    throw 'Baseline probe substitutions unexpectedly changed the INI size.'
}

New-Item -ItemType Directory -Path $artifactPath | Out-Null
$stageDll = Join-Path $artifactPath 'd3d9.dll'
$stageDinput = Join-Path $artifactPath 'dinput8.dll'
$stageIni = Join-Path $artifactPath 'SingularityVR.ini'
Copy-Item -LiteralPath $candidateDll -Destination $stageDll
Copy-Item -LiteralPath $installedDinput -Destination $stageDinput
[System.IO.File]::WriteAllBytes($stageIni, $candidateIniBytes)

$candidateDllSha = (Get-FileHash -LiteralPath $stageDll -Algorithm SHA256).Hash.ToUpperInvariant()
$candidateDinputSha = (Get-FileHash -LiteralPath $stageDinput -Algorithm SHA256).Hash.ToUpperInvariant()
$candidateIniSha = (Get-FileHash -LiteralPath $stageIni -Algorithm SHA256).Hash.ToUpperInvariant()
$sourceIniSha = (Get-FileHash -LiteralPath $installedIni -Algorithm SHA256).Hash.ToUpperInvariant()
if ($candidateDinputSha -ne $expectedDinput) { throw 'Staged dinput8.dll hash changed during copy.' }
if ($candidateDllSha -eq $expectedDll) { throw 'Candidate d3d9.dll unexpectedly equals the accepted DLL.' }

$plan = [ordered]@{
    schema = 'singularityvr-change-plan-v1'
    profileSha256 = $profileSha
    reason = $planReason
    files = @(
        [ordered]@{
            path = 'Binaries/d3d9.dll'
            before = $expectedDll
            after = $candidateDllSha
            reason = 'MAIN build from current origin/main with explicit exact-pair native muzzle selection; bounded probe for L3 capture.'
        }
    )
    ini = @($iniChanges.ToArray())
}
$planPath = Join-Path $artifactPath 'change-plan-native-muzzle-probe.json'
$plan | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $planPath -Encoding UTF8

$report = [ordered]@{
    acceptedProfile = $profileFullPath
    acceptedProfileSha256 = $profileSha
    installedIniSha256 = $sourceIniSha
    candidateIniSha256 = $candidateIniSha
    installedD3d9Sha256 = $actualDll
    candidateD3d9Sha256 = $candidateDllSha
    installedDinput8Sha256 = $actualDinput
    candidateDinput8Sha256 = $candidateDinputSha
    iniChanges = @($iniChanges.ToArray())
    steamWritten = $false
    changePlan = $planPath
}
$report | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $artifactPath 'candidate-manifest.json') -Encoding UTF8
Write-Host 'Native muzzle candidate package prepared offline; Steam was not modified.'
Write-Host "Artifact: $artifactPath"
Write-Host "Candidate d3d9.dll SHA-256: $candidateDllSha"
Write-Host "Candidate dinput8.dll SHA-256: $candidateDinputSha (unchanged accepted file)"
Write-Host "Candidate SingularityVR.ini SHA-256: $candidateIniSha (probe=$probeAfter, mode=$modeAfter)"
Write-Host "Change plan: $planPath"
