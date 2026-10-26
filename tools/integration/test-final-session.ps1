# Real production deployment and final-session guard against a non-executable
# fake Steam tree. No game launch and no files in the real installation.
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
. "$repo/tools/deployment/AcceptedProfileGuard.ps1"
$base=Join-Path $repo ('build/next-weapon-reload/final-lease-fixture-'+[guid]::NewGuid().ToString('N'))
$game="$base/steam/steamapps/common/Singularity";$bin="$game/Binaries"
$candidate="$base/candidate";$stage="$candidate/runtime";$assets="$base/assets-backup"
[void][IO.Directory]::CreateDirectory($bin);[void][IO.Directory]::CreateDirectory($stage)
function Put($path,$value){[void][IO.Directory]::CreateDirectory((Split-Path -Parent $path));[IO.File]::WriteAllText($path,$value)}
function Json($path,$value){Put $path ($value|ConvertTo-Json -Depth 12)}
function Hash($path){Get-GuardHash $path}
function Check($ok,$why){if(-not $ok){throw $why}}
function Refuse([scriptblock]$call){$failed=$false;try{& $call|Out-Null}catch{$failed=$true};Check $failed 'Expected guarded refusal'}
$global:finalLeaseFixtureRunning=$false
function Get-CimInstance {
    param($ClassName,$Filter,$ErrorAction)
    if($Filter -like 'ProcessId=*'){return [pscustomobject]@{ProcessId=$PID}}
    if($global:finalLeaseFixtureRunning){return [pscustomobject]@{ProcessId=12345;Name='Singularity.exe'}}
    return @()
}
function Get-Process {
    param($Id,$Name,$ErrorAction)
    if($Id){return [pscustomobject]@{Id=$PID}}
    if($global:finalLeaseFixtureRunning){return [pscustomobject]@{Id=12345;Name='Singularity'}}
    return @()
}
Put "$bin/Singularity.exe" 'NON-EXECUTABLE FIXTURE'
$pe=[byte[]]::new(512);$pe[0]=77;$pe[1]=90;$pe[60]=128
$pe[128]=80;$pe[129]=69;$pe[132]=76;$pe[133]=1;$pe[152]=11;$pe[153]=1
foreach($name in @('d3d9.dll','dinput8.dll')){
    [IO.File]::WriteAllBytes("$bin/$name",$pe)
    $next=[byte[]]$pe.Clone();$next[511]=1;[IO.File]::WriteAllBytes("$stage/$name",$next)
}
$ini="[Render]`nGloveNativeEnabled=0`nGloveRendererEnabled=0`nKnifeHandGripEnabled=0`nHandThumbPoseFix=0`nKnifeSceneLighting=0`n[Input]`nHipKnifeEnabled=0`n[Localization]`nPolishLanguage=0`n"
Put "$bin/SingularityVR.ini" $ini;Put "$stage/SingularityVR.ini" ($ini+"[Diagnostic]`nFixture=1`n")
Put "$bin/texture_override.ini" "[general]`nvalkyrie_material_mode=0`n"
Put "$stage/texture_override.ini" "[general]`nvalkyrie_material_mode=2`n"
$files=@()
foreach($relative in @('RvGame/CookedPC/UI_SinglePlayer_SF.xxx','Binaries/texture-override-remaster/ar_sniper_rifle_d.dds','Binaries/texture-override-remaster/revert_d2.dds')){
    if($relative.EndsWith('.xxx')){Put "$game/$relative" 'native menu fixture'}
    Put "$candidate/payload/$relative" ('candidate '+$relative)
    if($relative.EndsWith('revert_d2.dds')){Put "$game/$relative" ('candidate '+$relative)}
    $files += [pscustomobject]@{path=$relative;before=(Hash "$game/$relative");after=(Hash "$candidate/payload/$relative").ToLowerInvariant()}
}
foreach($row in $files){if($row.before){$row.before=$row.before.ToLowerInvariant()}}
Put "$game/RvGame/CookedPC/RvGame.xxx" 'accepted original core fixture'
Json "$candidate/manifest.json" @{schema='singularityvr-session-assets-delta-v1';files=$files;protected=@(
 @{path='Binaries/Singularity.exe';sha256=(Hash "$bin/Singularity.exe").ToLowerInvariant()},
 @{path='RvGame/CookedPC/RvGame.xxx';sha256=(Hash "$game/RvGame/CookedPC/RvGame.xxx").ToLowerInvariant()})}
$pins=@(@('d3d9.dll','dinput8.dll','texture_override.ini','Singularity.exe')|ForEach-Object{@{path='Binaries/'+$_;sha256=(Hash "$bin/$_")}})
$pins+=@($files|ForEach-Object{@{path=$_.path;sha256=$_.before}})
$pins+=@{path='RvGame/CookedPC/RvGame.xxx';sha256=(Hash "$game/RvGame/CookedPC/RvGame.xxx")}
Json "$base/profile.json" @{schema='singularityvr-accepted-profile-v1';approved=$true;acceptedBy='synthetic fixture';acceptanceScope='offline fake Steam only';iniPath='Binaries/SingularityVR.ini';ini=(Read-GuardIni "$bin/SingularityVR.ini");ignoredIniKeys=@();files=$pins}
$changes=@(@('d3d9.dll','dinput8.dll','texture_override.ini')|ForEach-Object{@{path='Binaries/'+$_;before=(Hash "$bin/$_");after=(Hash "$stage/$_");reason='fixture'}})
$plan=@{schema='singularityvr-change-plan-v1';profileSha256=(Hash "$base/profile.json");reason='fixture';files=$changes;ini=@(@{key='Diagnostic/Fixture';before=$null;after='1';reason='fixture'})}
Json "$base/core.json" $plan
$combined=$plan|ConvertTo-Json -Depth 12|ConvertFrom-Json
foreach($row in $files){if($row.before -ine $row.after){$combined.files+=@{path=$row.path;before=$row.before;after=$row.after;reason='fixture'}}}
Json "$base/combined.json" $combined
$iniHash=Hash "$bin/SingularityVR.ini"
$log=& pwsh -NoProfile -File "$repo/tools/deploy-singularityvr.ps1" -SteamRoot "$base/steam" -RepoRoot $repo `
    -ArtifactDir $stage -AcceptedProfilePath "$base/profile.json" -ChangePlanPath "$base/core.json" `
    -RuntimeDllPairOnly -IncludeIni -IncludeTextureIni 2>&1
Check ($LASTEXITCODE -eq 0) "Runtime fixture install failed: $log"
$backup=@($log|Where-Object{$_ -like 'Backup: *'})[-1].ToString().Substring(8)
& "$PSScriptRoot/session-assets.ps1" -Action Deploy -ManifestPath "$candidate/manifest.json" -BackupDir $assets -GameRoot $game
$args=@{CandidateDir=$candidate;RuntimeBackup=$backup;AssetBackup=$assets;ProfilePath="$base/profile.json";
 CorePlan="$base/core.json";CombinedPlan="$base/combined.json";BaselineIniSha256=$iniHash;SteamRoot="$base/steam"}
$watch="$PSScriptRoot/watch-final-session.ps1"
& $watch @args -ValidateOnly
function CandidateIntact {
    foreach($row in $files){Check ((Hash "$game/$($row.path)") -ieq $row.after) 'Refusal partially restored assets'}
    Check ((Hash "$bin/d3d9.dll") -ieq (Hash "$stage/d3d9.dll")) 'Refusal partially restored runtime'
}
$global:finalLeaseFixtureRunning=$true
Refuse {& $watch @args -RestoreNow};CandidateIntact
$global:finalLeaseFixtureRunning=$false
$saved=[IO.File]::ReadAllBytes("$game/RvGame/CookedPC/RvGame.xxx")
Put "$game/RvGame/CookedPC/RvGame.xxx" 'foreign core package'
Refuse {& $watch @args -RestoreNow};CandidateIntact
[IO.File]::WriteAllBytes("$game/RvGame/CookedPC/RvGame.xxx",$saved)
$saved=[IO.File]::ReadAllBytes("$backup/dinput8.dll")
Put "$backup/dinput8.dll" 'corrupt bridge preimage'
Refuse {& $watch @args -RestoreNow};CandidateIntact
[IO.File]::WriteAllBytes("$backup/dinput8.dll",$saved)
$saved=[IO.File]::ReadAllBytes("$assets/$($files[0].path)")
Put "$assets/$($files[0].path)" 'corrupt menu preimage'
Refuse {& $watch @args -RestoreNow};CandidateIntact
[IO.File]::WriteAllBytes("$assets/$($files[0].path)",$saved)
$saved=[IO.File]::ReadAllBytes("$bin/texture_override.ini")
Put "$bin/texture_override.ini" 'foreign texture configuration'
Refuse {& $watch @args -RestoreNow};CandidateIntact
[IO.File]::WriteAllBytes("$bin/texture_override.ini",$saved)
& $watch @args -RestoreNow
foreach($pin in $pins){Check ((Hash "$game/$($pin.path)") -ieq $pin.sha256) 'Exact baseline restoration failed'}
Check ((Hash "$bin/SingularityVR.ini") -ieq $iniHash) 'Exact INI restoration failed'
Remove-Variable finalLeaseFixtureRunning -Scope Global
Write-Output 'PASS: production four-file + assets install, ValidateOnly, running refusal, protected core drift, corrupt runtime/asset backup and live config refusal BEFORE writes, exact restore including absent DDS. Fake Steam only.'
