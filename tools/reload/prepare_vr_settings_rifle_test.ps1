[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$InstalledIni,
    [Parameter(Mandatory)][string]$AcceptedProfilePath,
    [Parameter(Mandatory)][string]$CandidateDir,
    [ValidateSet('Valkyrie','Volk','Kasimov')][string]$Weapon='Valkyrie',
    [switch]$CarryRayFix,
    [switch]$NativeFxCapture,
    [switch]$VisualFollowup,
    [switch]$MinimalSettings
)
$ErrorActionPreference='Stop'
$candidate=[IO.Path]::GetFullPath($CandidateDir)
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
if(-not $candidate.StartsWith((Join-Path $repo 'build').TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase)){
    throw 'CandidateDir must be in this checkout/build'
}
$dll=Join-Path $candidate 'd3d9.dll'
$manifest=Join-Path $candidate 'd3d9.build.json'
$iniOut=Join-Path $candidate 'SingularityVR.ini'
$planOut=Join-Path $candidate 'change-plan.json'
if(-not (Test-Path -LiteralPath $dll -PathType Leaf) -or
   (Test-Path -LiteralPath $iniOut) -or (Test-Path -LiteralPath $planOut)){
    throw 'Candidate is missing its DLL or already has test configuration'
}
$build=Get-Content -Raw -LiteralPath $manifest | ConvertFrom-Json
$dllHash=(Get-FileHash -LiteralPath $dll -Algorithm SHA256).Hash.ToUpperInvariant()
if($dllHash -cne $build.sha256 -or $build.architecture -cne 'x86 PE32' -or
   $build.optimization -cne '/O2' -or @($build.workingTreeStatus).Count){
    throw 'Clean optimized build receipt mismatch'
}
$profile=Get-Content -Raw -LiteralPath $AcceptedProfilePath | ConvertFrom-Json
$profileHash=(Get-FileHash -LiteralPath $AcceptedProfilePath -Algorithm SHA256).Hash.ToUpperInvariant()
$acceptedDll=@($profile.files | Where-Object path -ceq 'Binaries/d3d9.dll')
if($profile.schema -cne 'singularityvr-accepted-profile-v1' -or -not $profile.approved -or
   $acceptedDll.Count -ne 1){throw 'Accepted profile identity invalid'}
$targets=[ordered]@{
    # Preserve the already accepted native particle correction. Omitting this
    # default-OFF key caused the 2026-10-18 headset VFX regression.
    'Render/NativeMuzzleParticleStereo'='1'
    # SteamVR imports the completed XR surface, not the desktop F12 image.
    # Transparent XR alpha accumulated stale HUD frames in the user screenshot;
    # the opaque-alpha path was physically accepted on 2026-10-18.
    'Render/XrCaptureOpaqueAlpha'='1'
    'Render/VrPromptTokens'='1'
    'Render/DesktopMirrorWindow'='1'
    # 65.625 UU/m: center moves from ~27 to ~21 cm off the torso; the store
    # sphere shrinks from ~24 to ~14 cm. Keep the proven draw tolerance.
    'Input/HolsterRight'='14'
    'Input/HolsterRadius'='9'
    'Input/HipKnifeLeftCm'='20'
    'Input/HipKnifeRadiusCm'='18'
    # Preserve the user's approved right-handed Sense layout: Circle is the
    # manual reload edge, Cross remains health, and stick-up remains jump.
    'Input/ReloadControlLayout'='1'
    # The reload FSM is a single active transaction; exactly one weapon is
    # opted in per Steam session. Pass -Weapon to prepare another isolated run.
    'Input/ValkyrieManualReload'=$(if($Weapon -eq 'Valkyrie'){'1'}else{'0'})
    'Input/VolkManualReload'=$(if($Weapon -eq 'Volk'){'1'}else{'0'})
    'Input/KasimovManualReload'=$(if($Weapon -eq 'Kasimov'){'1'}else{'0'})
    'Input/ReloadRefillOnly'='1'
    'Input/ReloadNativeMagazine'='1'
    'Input/ValkyrieAuthoredGrip'=$(if($Weapon -eq 'Valkyrie'){'1'}else{'0'})
    'Input/ValkyrieSingleGrip'=$(if($Weapon -eq 'Valkyrie'){'1'}else{'0'})
}
if($CarryRayFix){
    $targets['Input/ReloadPalmGrip']='1'
    $targets['Input/WeaponTraceRigidOrigin']='1'
}
if($NativeFxCapture){
    # Existing automatic idle + three-shot trace, no readback/capture BMPs.
    # Preserve the accepted exact drop and particle repair; observation only.
    $targets['Diagnostic/NativeMuzzleProbe']='1'
}
if($VisualFollowup){
    $targets['Input/VolkAuthoredGrip']='1'
    $targets['Input/WeaponWheelObservedSelection']='1'
    $targets['Input/WeaponWheelDirectSelect']='1'
}
if($MinimalSettings){
    $targets['Render/VrSettingsMinimal']='1'
    $targets['Input/ReloadHints']='0'
    # Toggling test settings must not invalidate the exact-hash restore lease.
    # Normal production settings persist; this session alone is non-persistent.
    $targets['Diagnostic/VrSettingsSessionOnly']='1'
}
$text=[IO.File]::ReadAllText((Resolve-Path -LiteralPath $InstalledIni).Path,[Text.UTF8Encoding]::new($false,$true))
$newline=if($text.Contains("`r`n")){"`r`n"}else{"`n"}
$lines=[Collections.Generic.List[string]]::new()
foreach($line in [regex]::Split($text,'\r\n|\n|\r')){$lines.Add($line)}
$section='';$seen=@{};$sectionEnd=@{}
for($i=0;$i -lt $lines.Count;$i++){
    if($lines[$i] -match '^\s*\[([^\]]+)\]\s*$'){$section=$Matches[1]}
    if($section){$sectionEnd[$section]=$i}
    if($lines[$i] -match '^\s*([^;#\s][^=]*?)\s*='){
        $name=$section+'/'+$Matches[1].Trim()
        if($targets.Contains($name)){
            if($seen.ContainsKey($name)){throw "Duplicate input INI key: $name"}
            $seen[$name]=$true;$lines[$i]=$Matches[1].Trim()+'='+$targets[$name]
        }
    }
}
foreach($name in $targets.Keys){
    if($seen.ContainsKey($name)){continue}
    $parts=$name.Split('/');$section=$parts[0]
    if(-not $sectionEnd.ContainsKey($section)){throw "Missing INI section: $section"}
    $lines.Insert([int]$sectionEnd[$section]+1,$parts[1]+'='+$targets[$name])
    foreach($key in @($sectionEnd.Keys)){if($sectionEnd[$key] -ge $sectionEnd[$section]){$sectionEnd[$key]++}}
}
$output=$lines -join $newline
[IO.File]::WriteAllText($iniOut,$output,[Text.UTF8Encoding]::new($false))
$changes=@()
foreach($name in $targets.Keys){
    $before=$profile.ini.PSObject.Properties[$name]
    if($before -and [string]::Equals([string]$before.Value,[string]$targets[$name],[StringComparison]::Ordinal)){
        continue
    }
    $changes += [pscustomobject]@{
        key=$name
        before=if($before){$before.Value}else{$null}
        after=$targets[$name]
        reason="Isolated $Weapon manual magazine PSVR2 session; other rifle reload gates OFF; accepted profile restored after test"
    }
}
$plan=[pscustomobject]@{
    schema='singularityvr-change-plan-v1'
    profileSha256=$profileHash
    reason="Verify $Weapon guarded reload with the required Circle input layout, while preserving the tested VFX/holster/VR-settings candidate"
    files=@([pscustomobject]@{path='Binaries/d3d9.dll';before=$acceptedDll[0].sha256;after=$dllHash;reason='Clean x86 /O2 candidate'})
    ini=$changes
}
$plan | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $planOut -Encoding UTF8
Write-Output "Prepared DLL/INI change plan outside Steam: $candidate"
Get-FileHash -Algorithm SHA256 $dll,$iniOut,$planOut | Select-Object Hash,Path
