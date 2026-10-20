[CmdletBinding()]
param(
 [Parameter(Mandatory)][string]$InstalledIni,
 [Parameter(Mandatory)][string]$AcceptedProfilePath,
 [Parameter(Mandatory)][string]$CandidateDir,
 [switch]$RifleSniperFollowups
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
 if(-not $end.ContainsKey($section)){throw "Missing section: $section"}
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
