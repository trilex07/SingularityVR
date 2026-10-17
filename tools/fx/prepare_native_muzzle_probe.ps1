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
    [switch]$PathWitness,
    [switch]$NativeFxOffControl,
    [switch]$ObserveRoute,
    [switch]$WorldRoute,
    [switch]$ParticleStereo,
    [switch]$ProjectorStereo,
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
if ($ProjectorStereo -and -not $ParticleStereo) { throw 'Projector test must preserve the physically accepted particle correction via -ParticleStereo.' }
if ($ProjectorStereo -and $iniValues.ContainsKey('Render/ProjectorWorldStereo')) { throw 'Projector key already exists; refusing an unknown baseline.' }
if ($NoProbe -and -not ($dropRequested -or $PathWitness -or $ParticleStereo)) {
    throw '-NoProbe is only valid for an exact-pair visual discriminator; baseline capture requires the bounded probe.'
}
if ($ParticleStereo) {
    if (-not $NoProbe -or $dropRequested -or $PathWitness -or $WorldRoute -or $ObserveRoute -or $NativeFxOffControl) {
        throw '-ParticleStereo requires -NoProbe and is exclusive of other experimental controls.'
    }
    $particleKey = 'Render/NativeMuzzleParticleStereo'
    if ($iniValues.ContainsKey($particleKey) -or @($profile.ini.PSObject.Properties | Where-Object { $_.Name -ceq $particleKey }).Count) {
        throw 'Particle stereo key already exists; refusing to overwrite an unknown setting.'
    }
    foreach ($key in @('Diagnostic/NativeMuzzleVSHash','Diagnostic/NativeMuzzlePSHash','Diagnostic/NativeMuzzleVSBytes','Diagnostic/NativeMuzzlePSBytes','Render/HideMuzzleFx','Render/VfxHost')) {
        if ($iniValues[$key] -cne (Get-ProfileIniValue $key)) { throw "Installed particle baseline differs from profile: $key" }
    }
    if ($iniValues['Render/HideMuzzleFx'] -ne '0' -or $iniValues['Render/VfxHost'] -ne '0') {
        throw 'Particle test requires original emitter ON and generated VFX OFF.'
    }
}
if ($NativeFxOffControl) {
    if ($NoProbe -or $PathWitness -or $dropRequested) {
        throw '-NativeFxOffControl is a bounded probe-only control; do not combine with a drop or path witness.'
    }
    foreach ($key in @('Render/HideMuzzleFx','Render/VfxHost')) {
        if (-not $iniValues.ContainsKey($key) -or $iniValues[$key] -cne (Get-ProfileIniValue $key)) {
            throw "Installed INI differs from accepted profile at $key."
        }
    }
    if ($iniValues['Render/HideMuzzleFx'] -ne '0' -or $iniValues['Render/VfxHost'] -ne '0') {
        throw 'Native-FX-off control requires native ON and generated VFX OFF in the accepted installation.'
    }
}
if ($ObserveRoute -and (-not $PathWitness -or -not $NoProbe -or $NativeFxOffControl)) {
    throw '-ObserveRoute requires -PathWitness -NoProbe and cannot suppress the native emitter.'
}
if ($WorldRoute -and ($ObserveRoute -or $NativeFxOffControl -or -not $PathWitness -or -not $NoProbe -or -not $dropRequested)) {
    throw '-WorldRoute requires one exact shader pair, -PathWitness and -NoProbe; it cannot combine with other controls.'
}
if ($WorldRoute) {
    foreach ($key in @('Diagnostic/NativeMuzzleWorldRoute','Diagnostic/NativeMuzzleWorldVSHash',
        'Diagnostic/NativeMuzzleWorldVSBytes','Diagnostic/NativeMuzzleWorldPSHash','Diagnostic/NativeMuzzleWorldPSBytes')) {
        if ($iniValues.ContainsKey($key) -or
            @($profile.ini.PSObject.Properties | Where-Object { $_.Name -ceq $key }).Count) {
            throw "World-route key already exists; refusing to overwrite: $key"
        }
    }
}
if ($PathWitness) {
    if (-not $NoProbe) { throw '-PathWitness requires -NoProbe.' }
    if ($iniValues.ContainsKey('Diagnostic/NativeMuzzlePathWitness') -or
        @($profile.ini.PSObject.Properties | Where-Object { $_.Name -ceq 'Diagnostic/NativeMuzzlePathWitness' }).Count) {
        throw 'Path witness key already exists; refusing to overwrite an unknown setting.'
    }
    foreach ($key in @('Diagnostic/NativeMuzzleVSHash','Diagnostic/NativeMuzzlePSHash',
        'Diagnostic/NativeMuzzleVSBytes','Diagnostic/NativeMuzzlePSBytes')) {
        if ($iniValues[$key] -cne (Get-ProfileIniValue $key)) { throw "Installed selected pair differs from accepted profile: $key" }
    }
}
if ($dropRequested) {
    if (-not $DropVSHash -or -not $DropPSHash -or $DropVSBytes -le 0 -or $DropPSBytes -le 0) {
        throw 'An exact-pair candidate needs all four values: VS/PS hashes and byte lengths.'
    }
    if ($DropVSHash -notmatch '^[0-9A-Fa-f]{16}$' -or $DropPSHash -notmatch '^[0-9A-Fa-f]{16}$') {
        throw 'Drop shader hashes must each be exactly 16 hexadecimal characters.'
    }
    foreach ($size in @($DropVSBytes, $DropPSBytes)) {
        if ($size -lt 8 -or $size -gt 32768 -or ($size % 4) -ne 0) {
            throw "Invalid drop shader byte length: $size."
        }
    }
    if ($modeBefore -ne '1') { throw 'An exact-pair candidate requires the accepted baseline mode=1.' }
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
if ($ParticleStereo) {
    $modeAfter = $modeBefore
    $newline = if ($iniText.Contains("`r`n")) { "`r`n" } else { "`n" }
    $candidateIniBytes = Replace-UniqueAscii $candidateIniBytes '[Render]' "[Render]${newline}NativeMuzzleParticleStereo=1"
    $iniChanges.Add([ordered]@{ key = 'Render/NativeMuzzleParticleStereo'; before = $null; after = '1'; reason = 'Route all three audited original particle pairs through VP stereo, preserve LocalToWorld; bypass accepted drop only inside validated corrected draw scope.' })
    $planReason = 'MAIN particle family coordinate-space correction: exact three native shader pairs, default OFF, c0 ROW stereo, c5-c8 unchanged; bounded restoration witness, no readback capture or generated FX.'
} elseif ($PathWitness) {
    $modeAfter = if ($ObserveRoute) { '3' } else { $modeBefore }
    $newline = if ($iniText.Contains("`r`n")) { "`r`n" } else { "`n" }
    $worldKeys = if ($WorldRoute) {
        "NativeMuzzleWorldRoute=1${newline}NativeMuzzleWorldVSHash=$($DropVSHash.ToUpperInvariant())${newline}NativeMuzzleWorldVSBytes=$DropVSBytes${newline}NativeMuzzleWorldPSHash=$($DropPSHash.ToUpperInvariant())${newline}NativeMuzzleWorldPSBytes=$DropPSBytes${newline}"
    } else { '' }
    $candidateIniBytes = Replace-UniqueAscii $candidateIniBytes "NativeMuzzleStereoMode=$modeBefore" "NativeMuzzlePathWitness=1${newline}${worldKeys}NativeMuzzleStereoMode=$modeAfter"
    $iniChanges.Add([ordered]@{ key = 'Diagnostic/NativeMuzzlePathWitness'; before = $null; after = '1'; reason = 'Bounded read-only selected-pair draw-path census; no renderer behavior change.' })
    if ($ObserveRoute) {
        $iniChanges.Add([ordered]@{ key = $modeKey; before = $modeBefore; after = $modeAfter; reason = 'Observe-only exact-pair mode; no shader drops or matrix edits.' })
    }
    if ($WorldRoute) {
        $worldTargets = [ordered]@{
            'Diagnostic/NativeMuzzleWorldRoute' = '1'
            'Diagnostic/NativeMuzzleWorldVSHash' = $DropVSHash.ToUpperInvariant()
            'Diagnostic/NativeMuzzleWorldVSBytes' = [string]$DropVSBytes
            'Diagnostic/NativeMuzzleWorldPSHash' = $DropPSHash.ToUpperInvariant()
            'Diagnostic/NativeMuzzleWorldPSBytes' = [string]$DropPSBytes
        }
        foreach ($key in $worldTargets.Keys) {
            $iniChanges.Add([ordered]@{ key = $key; before = $null; after = [string]$worldTargets[$key]; reason = 'Separate exact world-particle route; accepted mode-1 pair and drop remain unchanged.' })
        }
        $planReason = 'MAIN L3 exact world-route experiment with accepted mode-1 drop preserved; only the separately selected world particle changes path; readbacks OFF.'
    } elseif ($dropRequested) {
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
                $pairReason = if ($WorldRoute) { 'Exact world-particle shader identity measured by causal emitter ON/OFF trace and selected draw-path witness.' } else { 'Causal emitter-ON/OFF traces isolated this additive shader pair for exact-path testing.' }
                $iniChanges.Add([ordered]@{ key = $key; before = $before; after = $after; reason = $pairReason })
            }
        }
        $planReason = if ($ObserveRoute) {
            'MAIN L3 read-only exact additive-pair route witness after causal RvWeaponFX ON/OFF comparison; no drops, alias edits or GPU readbacks.'
        } else {
            'MAIN L3 exact additive-pair drop plus bounded path witness after causal RvWeaponFX ON/OFF comparison; probe/readbacks OFF.'
        }
    } else {
        $planReason = 'MAIN L3 read-only native muzzle path witness for the accepted exact pair; preserve accepted mode-1 drop and disable frame readbacks.'
    }
} elseif ($dropRequested) {
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
    if ($NativeFxOffControl) {
        $candidateIniBytes = Replace-UniqueAscii $candidateIniBytes 'HideMuzzleFx=0' 'HideMuzzleFx=1'
        $iniChanges.Add([ordered]@{ key = 'Render/HideMuzzleFx'; before = '0'; after = '1'; reason = 'Suppress only the proven RvWeaponFX emitter references to causally identify their GPU draws against the completed native-ON trace.' })
        $planReason = 'MAIN L3 causal native-FX-off control: bounded idle/three-shot trace with original muzzle emitter suppressed, generated host OFF, and no shader drops or alias repair.'
    } else {
        $planReason = 'MAIN L3 native muzzle capture: preserve accepted profile; set only NativeMuzzleProbe=1 and NativeMuzzleStereoMode=0 for native-only baseline traces.'
    }
}
if (-not $dropRequested -and -not $PathWitness -and -not $ParticleStereo -and $candidateIniBytes.Length -ne $iniBytes.Length) {
    throw 'Baseline probe substitutions unexpectedly changed the INI size.'
}

if ($ProjectorStereo) {
    $newline = if ($iniText.Contains("`r`n")) { "`r`n" } else { "`n" }
    $candidateIniBytes = Replace-UniqueAscii $candidateIniBytes '[Render]' "[Render]${newline}ProjectorWorldStereo=1"
    $iniChanges.Add([ordered]@{ key = 'Render/ProjectorWorldStereo'; before = $null; after = '1'; reason = 'Exact captured light-function pair: per-eye ScreenToLight reconstruction; retained accepted native particle stereo.' })
    $planReason = 'MAIN projector test: preserve accepted muzzle fix and change only the exact projector reconstruction versus preceding projector capture session. No performance changes.'
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
            reason = 'MAIN x86 build with bounded native-FX diagnostics and explicit exact-pair control; no unrelated renderer or gameplay change.'
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
