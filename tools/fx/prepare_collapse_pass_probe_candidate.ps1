[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$AcceptedProfilePath,
    [Parameter(Mandatory)][string]$SourceArtifactDir,
    [Parameter(Mandatory)][string]$ArtifactDir,
    [string]$SteamRoot = 'C:\Program Files (x86)\Steam'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$artifactFull = [IO.Path]::GetFullPath($ArtifactDir)
if (-not $artifactFull.StartsWith($repoRoot.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'ArtifactDir must be inside this MAIN worktree.'
}
if (Test-Path -LiteralPath $artifactFull) { throw "Refusing to overwrite artifact directory: $artifactFull" }
if (@(Get-Process -Name Singularity -ErrorAction SilentlyContinue).Count) {
    throw 'Singularity is running; refusing to prepare from a live installation.'
}

$profileFull = (Resolve-Path -LiteralPath $AcceptedProfilePath).Path
$profileObj = Get-Content -LiteralPath $profileFull -Raw | ConvertFrom-Json
if ($profileObj.schema -cne 'singularityvr-accepted-profile-v1' -or $profileObj.approved -ne $true) {
    throw 'A reviewed accepted profile is required.'
}
$profileSha = (Get-FileHash -LiteralPath $profileFull -Algorithm SHA256).Hash.ToUpperInvariant()
$sourceFull = (Resolve-Path -LiteralPath $SourceArtifactDir).Path
$sourceDll = Join-Path $sourceFull 'd3d9.dll'
$sourceDinput = Join-Path $sourceFull 'dinput8.dll'
$sourceDllShaExpected = 'D6D1284F858CEF36605903772773E005033899B18B26F16C6C180EB6D2B0B0E8'
if ((Get-FileHash -LiteralPath $sourceDll -Algorithm SHA256).Hash.ToUpperInvariant() -cne $sourceDllShaExpected) {
    throw 'The renderer DLL does not match the reviewed pass-probe build.'
}

$gameBin = Join-Path $SteamRoot 'steamapps\common\Singularity\Binaries'
$liveDll = Join-Path $gameBin 'd3d9.dll'
$liveDinput = Join-Path $gameBin 'dinput8.dll'
$liveIni = Join-Path $gameBin 'SingularityVR.ini'
$expectedFiles = @{}
foreach ($entry in $profileObj.files) { $expectedFiles[$entry.path] = ([string]$entry.sha256).ToUpperInvariant() }
foreach ($pair in @(@($liveDll, 'Binaries/d3d9.dll'), @($liveDinput, 'Binaries/dinput8.dll'))) {
    if ((Get-FileHash -LiteralPath $pair[0] -Algorithm SHA256).Hash.ToUpperInvariant() -cne $expectedFiles[$pair[1]]) {
        throw "Installed file is outside the accepted profile: $($pair[1])"
    }
}

function Read-IniMap([string]$Path) {
    $map = @{}; $section = ''
    foreach ($line in [IO.File]::ReadAllText($Path) -split '\r?\n') {
        $trimmed = $line.Trim()
        if (-not $trimmed -or $trimmed.StartsWith(';') -or $trimmed.StartsWith('#')) { continue }
        if ($trimmed -match '^\[([^\]]+)\]$') { $section = $Matches[1]; continue }
        if ($trimmed -match '^([^=]+)=(.*)$') {
            $key = $section + '/' + $Matches[1].Trim()
            if ($map.ContainsKey($key)) { throw "Duplicate INI key: $key" }
            $map[$key] = $Matches[2].Trim(); continue
        }
        throw "Unparsed INI line: $trimmed"
    }
    return $map
}

$liveIniMap = Read-IniMap $liveIni
$profileIni = @{}
foreach ($property in $profileObj.ini.PSObject.Properties) { $profileIni[$property.Name] = [string]$property.Value }
$ignored = @($profileObj.ignoredIniKeys)
foreach ($key in @(@($liveIniMap.Keys) + @($profileIni.Keys) | Sort-Object -Unique)) {
    if ($key -notin $ignored -and [string]$liveIniMap[$key] -cne [string]$profileIni[$key]) {
        throw "Installed INI is outside the accepted profile at $key"
    }
}
foreach ($key in @('Diagnostic/FrameGpuProbe', 'Diagnostic/FrameGpuPassProbe')) {
    if ($liveIniMap.ContainsKey($key)) { throw "Probe key already exists: $key" }
}

$iniBytes = [IO.File]::ReadAllBytes($liveIni)
$iniText = [Text.UTF8Encoding]::new($false, $true).GetString($iniBytes)
$newline = if ($iniText.Contains("`r`n")) { "`r`n" } else { "`n" }
$needle = '[Diagnostic]'
$needleBytes = [Text.Encoding]::ASCII.GetBytes($needle)
$hits = 0
for ($i = 0; $i -le $iniBytes.Length - $needleBytes.Length; $i++) {
    $same = $true
    for ($j = 0; $j -lt $needleBytes.Length; $j++) { if ($iniBytes[$i + $j] -ne $needleBytes[$j]) { $same = $false; break } }
    if ($same) { $hits++ }
}
if ($hits -ne 1) { throw "Expected one Diagnostic section, found $hits" }
$candidateText = $iniText.Replace($needle, "$needle${newline}FrameGpuProbe=1${newline}FrameGpuPassProbe=1")
$tempIni = Join-Path $env:TEMP ('singularityvr-pass-probe-' + [Guid]::NewGuid().ToString('N') + '.ini')
try {
    [IO.File]::WriteAllText($tempIni, $candidateText, [Text.UTF8Encoding]::new($false))
    $candidateMap = Read-IniMap $tempIni
    foreach ($key in @(@($candidateMap.Keys) + @($profileIni.Keys) | Sort-Object -Unique)) {
        if ($key -notin $ignored -and $key -notin @('Diagnostic/FrameGpuProbe', 'Diagnostic/FrameGpuPassProbe') -and
            [string]$candidateMap[$key] -cne [string]$profileIni[$key]) {
            throw "Candidate changes accepted setting $key"
        }
    }
    if ($candidateMap['Diagnostic/FrameGpuProbe'] -cne '1' -or $candidateMap['Diagnostic/FrameGpuPassProbe'] -cne '1') {
        throw 'Candidate probe flags were not staged correctly.'
    }

    New-Item -ItemType Directory -Path $artifactFull | Out-Null
    $candidateDll = Join-Path $artifactFull 'd3d9.dll'
    $candidateDinput = Join-Path $artifactFull 'dinput8.dll'
    $candidateIni = Join-Path $artifactFull 'SingularityVR.ini'
    Copy-Item -LiteralPath $sourceDll -Destination $candidateDll
    Copy-Item -LiteralPath $liveDinput -Destination $candidateDinput
    Copy-Item -LiteralPath $tempIni -Destination $candidateIni

    $dllBefore = $expectedFiles['Binaries/d3d9.dll']
    $dllAfter = (Get-FileHash -LiteralPath $candidateDll -Algorithm SHA256).Hash.ToUpperInvariant()
    $iniAfter = (Get-FileHash -LiteralPath $candidateIni -Algorithm SHA256).Hash.ToUpperInvariant()
    $plan = [ordered]@{
        schema = 'singularityvr-change-plan-v1'
        profileSha256 = $profileSha
        reason = 'Read-only frame GPU phase attribution at the collapse scene; current accepted INI preserved, only the two bounded frame pass probe switches are enabled.'
        files = @([ordered]@{ path = 'Binaries/d3d9.dll'; before = $dllBefore; after = $dllAfter; reason = 'Install the reviewed pass-phase diagnostic build; no visual/gameplay change intended.' })
        ini = @(
            [ordered]@{ key = 'Diagnostic/FrameGpuProbe'; before = $null; after = '1'; reason = 'Enable existing asynchronous frame GPU timestamp collection.' },
            [ordered]@{ key = 'Diagnostic/FrameGpuPassProbe'; before = $null; after = '1'; reason = 'Attribute bounded frames by actual render target and draw kind; no per-shader GPU timing claim.' }
        )
    }
    $planPath = Join-Path $artifactFull 'change-plan-native-muzzle-probe.json'
    $plan | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $planPath -Encoding UTF8
    $manifest = [ordered]@{
        sourceArtifact = $sourceFull
        sourceDllSha256 = $dllAfter
        acceptedProfile = $profileFull
        acceptedProfileSha256 = $profileSha
        installedDllSha256 = $dllBefore
        installedDinputSha256 = $expectedFiles['Binaries/dinput8.dll']
        installedIniSha256 = (Get-FileHash -LiteralPath $liveIni -Algorithm SHA256).Hash.ToUpperInvariant()
        candidateIniSha256 = $iniAfter
        changedIniKeys = @('Diagnostic/FrameGpuProbe=1', 'Diagnostic/FrameGpuPassProbe=1')
        steamWritten = $false
    }
    $manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $artifactFull 'candidate-manifest.json') -Encoding UTF8
} finally {
    if (Test-Path -LiteralPath $tempIni) { Remove-Item -LiteralPath $tempIni -Force }
}

Write-Output "Prepared candidate: $artifactFull"
Write-Output "DLL SHA-256: $dllAfter"
Write-Output "INI SHA-256: $iniAfter"
Write-Output 'INI deltas: Diagnostic/FrameGpuProbe=1; Diagnostic/FrameGpuPassProbe=1'
