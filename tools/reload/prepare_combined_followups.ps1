[CmdletBinding()]
param(
 [Parameter(Mandatory)][string]$InstalledIni,
 [Parameter(Mandatory)][string]$AcceptedProfilePath,
 [Parameter(Mandatory)][string]$CandidateDir,
 [switch]$RifleSniperFollowups,
 [switch]$OpticalScopeAndRifleOrigin,
 [switch]$RifleTracerOrigin,
 [switch]$WeaponSteady,
 [switch]$CalibratedOptics,
 [switch]$MeasuredAim,
 [switch]$OpticalAccuracy,
 [switch]$StrongerPrecision,
 [switch]$ReviewedGore,
 [switch]$BodyEquipmentCensus
)
# Offline preparation only. No launch, installation or accepted-profile mutation.
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
. (Join-Path $repo 'tools/deployment/AcceptedProfileGuard.ps1')
$candidate=[IO.Path]::GetFullPath($CandidateDir)
if(-not $candidate.StartsWith((Join-Path $repo 'build').TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase)){
 throw 'CandidateDir must be in this checkout/build'
}
$iniOut=Join-Path $candidate 'SingularityVR.ini'
$planOut=Join-Path $candidate 'change-plan.json'
if((Test-Path -LiteralPath $iniOut) -or (Test-Path -LiteralPath $planOut)){throw 'Refusing existing candidate configuration'}
$dll=Join-Path $candidate 'd3d9.dll'
$build=Get-Content -Raw -LiteralPath (Join-Path $candidate 'd3d9.build.json') | ConvertFrom-Json
$hash=(Get-FileHash -LiteralPath $dll -Algorithm SHA256).Hash
if($build.sha256 -cne $hash -or $build.architecture -cne 'x86 PE32' -or
 $build.optimization -cne '/O2' -or @($build.workingTreeStatus).Count){throw 'Requires a matching clean x86 /O2 build'}
$profile=Get-Content -Raw -LiteralPath $AcceptedProfilePath | ConvertFrom-Json
$profileHash=(Get-FileHash -LiteralPath $AcceptedProfilePath -Algorithm SHA256).Hash
$iniPin=@($profile.files | Where-Object path -ceq 'Binaries/SingularityVR.ini')
$dllPin=@($profile.files | Where-Object path -ceq 'Binaries/d3d9.dll')
if($profile.schema -cne 'singularityvr-accepted-profile-v1' -or -not $profile.approved -or
 $iniPin.Count -gt 1 -or $dllPin.Count -ne 1 -or
 ($iniPin.Count -eq 1 -and (Get-FileHash -LiteralPath $InstalledIni -Algorithm SHA256).Hash -cne $iniPin[0].sha256)){
 throw 'Accepted profile / installed INI mismatch; no stale baseline adoption'
}
# Production profiles pin logical INI values (and explicitly ignored caches),
# not an INI file entry. Validate both directions without adopting unknown keys.
$actual=Read-GuardIni $InstalledIni
$ignored=@();if($profile.PSObject.Properties['ignoredIniKeys']){$ignored=@($profile.ignoredIniKeys)}
foreach($p in $profile.ini.PSObject.Properties){
 if($p.Name -in $ignored){continue}
 if(-not $actual.ContainsKey($p.Name) -or -not (Test-GuardSame $actual[$p.Name] $p.Value)){
  throw "Accepted INI value mismatch: $($p.Name)"
 }
}
foreach($key in $actual.Keys){
 if($key -in $ignored){continue}
 if(-not $profile.ini.PSObject.Properties[$key]){throw "Unknown installed INI key: $key"}
}
$targets=[ordered]@{
 'Input/KeepAcquiredWeapons'='1'
 'Input/WeaponWheelOwnedInventory'='1'
 'Input/WeaponWheelDirectSelect'='1'
 'Render/NpcNameScalePct'='60'
 'Render/CutsceneNativeHands'='1'
}
if($RifleSniperFollowups){
 $targets=[ordered]@{
  'Input/KasimovAuthoredGrip'='1'
  'Input/SniperNearEyeScope'='1'
  'Diagnostic/NativeMuzzleProbe'='1'
  'Diagnostic/NativeMuzzleProbeGunVerts'='11387'
 }
}
if($OpticalScopeAndRifleOrigin){
 if(-not $RifleSniperFollowups){throw 'Optical experiment requires -RifleSniperFollowups'}
 $targets['Render/SniperPictureInPicture']='1'
 $targets['Diagnostic/NativeRifleMuzzleOrigin']='3'
 # No additional four-frame pass dumps in the perceptual test. Bounded native
 # emitter / owned RTT receipts remain enabled by their experiment flags.
 $targets['Diagnostic/NativeMuzzleProbe']='0'
}
if($RifleTracerOrigin){
 if(-not $OpticalScopeAndRifleOrigin){throw 'Tracer experiment requires -OpticalScopeAndRifleOrigin'}
 $targets['Diagnostic/NativeRifleTracerOrigin']='1'
}
if($WeaponSteady){
 if(-not $OpticalScopeAndRifleOrigin){throw 'Weapon steady requires the optical candidate'}
 $targets['Input/WeaponSteady']='1'
}
if($CalibratedOptics){
 if(-not $WeaponSteady){throw 'Calibrated optics requires the steady optical candidate'}
 $targets['Render/ScopeOpticalCalibration']='1'
 $targets['Input/SniperOpticalSlowTime']='1'
}
if($MeasuredAim){
 if(-not $CalibratedOptics){throw 'Measured aim requires the calibrated optical candidate'}
 $targets['Input/WeaponMeasuredAim']='1'
 $targets['Input/WeaponTraceRigidOrigin']='1'
 $targets['Render/ScopeMeasuredPose']='1'
}
if($OpticalAccuracy){
 if(-not $MeasuredAim){throw 'Optical accuracy requires measured optics'}
 $targets['Input/SniperOpticalAccuracy']='1'
}
if($StrongerPrecision){
 if(-not $OpticalAccuracy -or -not $WeaponSteady){throw 'Stronger precision requires accepted optical accuracy and steady aim'}
 $targets['Input/WeaponSteadyPrecision']='2'
}
if($BodyEquipmentCensus){
 if(-not $MeasuredAim){throw 'Equipment census is scoped to the measured-aim followup'}
 $targets['Diagnostic/BodyEquipmentProbe']='1'
}
if($ReviewedGore){
 $targets['Gameplay/GoreDismemberment']='1'
 $targets['Gameplay/GoreChancePct']='100'
}
$text=[IO.File]::ReadAllText((Resolve-Path -LiteralPath $InstalledIni).Path,[Text.UTF8Encoding]::new($false,$true))
$newline=if($text.Contains("`r`n")){"`r`n"}else{"`n"}
$lines=[Collections.Generic.List[string]]::new()
foreach($line in [regex]::Split($text,'\r\n|\n|\r')){$lines.Add($line)}
$section='';$seen=@{};$end=@{}
for($i=0;$i -lt $lines.Count;$i++){
 if($lines[$i] -match '^\s*\[([^\]]+)\]\s*$'){$section=$Matches[1]}
 if($section){$end[$section]=$i}
 if($lines[$i] -match '^\s*([^;#\s][^=]*?)\s*='){
  $key=$section+'/'+$Matches[1].Trim()
  if($targets.Contains($key)){
   if($seen.ContainsKey($key)){throw "Duplicate input key: $key"}
   $seen[$key]=$true;$lines[$i]=$Matches[1].Trim()+'='+$targets[$key]
  }
 }
}
foreach($key in $targets.Keys){
 if($seen.ContainsKey($key)){continue}
 $parts=$key.Split('/');$section=$parts[0]
 if(-not $end.ContainsKey($section)){
  if($section -ne 'Gameplay' -or -not $ReviewedGore){throw "Missing section: $section"}
  $lines.Add('');$lines.Add('[Gameplay]');$end[$section]=$lines.Count-1
 }
 $at=[int]$end[$section]+1
 $lines.Insert($at,$parts[1]+'='+$targets[$key])
 foreach($s in @($end.Keys)){if($end[$s] -ge $at-1){$end[$s]++}}
}
$changes=@()
foreach($key in $targets.Keys){
 $before=$profile.ini.PSObject.Properties[$key]
 if($before -and [string]$before.Value -ceq $targets[$key]){continue}
 $changes += [pscustomobject]@{key=$key;before=$(if($before){$before.Value}else{$null});after=$targets[$key];reason='Combined followups; preserve promoted baseline otherwise'}
}
$plan=[pscustomobject]@{
 schema='singularityvr-change-plan-v1';profileSha256=$profileHash
 reason='All requested post-acceptance followups; only DLL and scoped INI keys; no campaign/package/asset deployment'
 files=@([pscustomobject]@{path='Binaries/d3d9.dll';before=$dllPin[0].sha256;after=$hash;reason='Clean x86 /O2 followup candidate'})
 ini=$changes
}
[IO.File]::WriteAllText($iniOut,($lines -join $newline),[Text.UTF8Encoding]::new($false))
$plan | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $planOut -Encoding UTF8
Write-Output "Prepared outside Steam: $candidate (no install or launch)"
