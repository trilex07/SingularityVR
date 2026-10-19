# Own-process fake installation; PRECHECK ONLY, never a live wait or restore.
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$root=Join-Path $repo ('build/combined-followups/lease-fixture-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $root | Out-Null
function Get-CimInstance {
 [CmdletBinding()]param([string]$ClassName,[string]$Filter)
 return [pscustomobject]@{ProcessId=$PID}
}
function Put($path,$value){[IO.File]::WriteAllText($path,$value,[Text.UTF8Encoding]::new($false))}
function Hash($path){(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash}
function MakeCase($name,$injector){
 $dir=Join-Path $root $name;$steam=Join-Path $dir 'steam'
 $bin=Join-Path $steam 'steamapps/common/Singularity/Binaries'
 $cooked=Join-Path $steam 'steamapps/common/Singularity/RvGame/CookedPC'
 $backup=Join-Path $dir 'backup';$candidate=Join-Path $dir 'candidate'
 foreach($p in @($bin,$cooked,$backup,$candidate)){New-Item -ItemType Directory -Force -Path $p | Out-Null}
 Put (Join-Path $backup 'd3d9.dll') 'accepted DLL';Put (Join-Path $backup 'SingularityVR.ini') 'accepted INI'
 Put (Join-Path $bin 'd3d9.dll') 'candidate DLL';Put (Join-Path $bin 'SingularityVR.ini') 'candidate INI'
 Put (Join-Path $candidate 'SingularityVR.ini') 'candidate INI';Put (Join-Path $cooked 'UI_SinglePlayer_SF.xxx') 'protected UI'
 $metadata=@();$files=@()
 foreach($n in @('d3d9.dll','SingularityVR.ini')){
  $metadata+=@{Name=$n;Sha256=(Hash (Join-Path $backup $n))}
 }
 $files+=@{path='Binaries/d3d9.dll';sha256=(Hash (Join-Path $backup 'd3d9.dll'))}
 if($injector){
  Put (Join-Path $backup 'dinput8.dll') 'accepted injector';Put (Join-Path $bin 'dinput8.dll') 'accepted injector'
  $h=Hash (Join-Path $backup 'dinput8.dll');$metadata+=@{Name='dinput8.dll';Sha256=$h}
  $files+=@{path='Binaries/dinput8.dll';sha256=$h}
 }
 @{Files=$metadata;Directories=@()} | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $backup 'backup_metadata.json') -Encoding UTF8
 $profile=Join-Path $dir 'profile.json'
 @{schema='singularityvr-accepted-profile-v1';approved=$true;files=$files} | ConvertTo-Json -Depth 6 | Set-Content $profile -Encoding UTF8
 $plan=Join-Path $candidate 'change-plan.json'
 @{schema='singularityvr-change-plan-v1';profileSha256=(Hash $profile);files=@(@{path='Binaries/d3d9.dll';before=(Hash (Join-Path $backup 'd3d9.dll'));after=(Hash (Join-Path $bin 'd3d9.dll'))})} | ConvertTo-Json -Depth 6 | Set-Content $plan -Encoding UTF8
 return @{BackupPath=$backup;AcceptedProfilePath=$profile;ChangePlanPath=$plan;
  AcceptedIniSha256=(Hash (Join-Path $backup 'SingularityVR.ini'));SteamRoot=$steam;PreflightOnly=$true}
}
$watch=Join-Path $repo 'tools/fx/watch_native_muzzle_restore.ps1'
$two=MakeCase 'two' $false;$three=MakeCase 'three' $true
& $watch @two -RuntimeDllOnly | Out-Null
& $watch @three | Out-Null
$checks=2
foreach($case in @(@{args=$two;narrow=$false},@{args=$three;narrow=$true})){
 $watchArgs=$case.args
 $failed=$false;try{& $watch @watchArgs -RuntimeDllOnly:$case.narrow | Out-Null}catch{$failed=$true}
 if(-not $failed){throw 'Wrong lease scope accepted'};$checks++
}
$bad=MakeCase 'bad-backup' $false
Put (Join-Path $bad.BackupPath 'd3d9.dll') 'corrupt backup'
$failed=$false;try{& $watch @bad -RuntimeDllOnly | Out-Null}catch{$failed=$true}
if(-not $failed){throw 'Corrupt backup accepted'};$checks++
$drift=MakeCase 'live-drift' $false
Put (Join-Path $drift.SteamRoot 'steamapps/common/Singularity/Binaries/d3d9.dll') 'other deployment'
$failed=$false;try{& $watch @drift -RuntimeDllOnly | Out-Null}catch{$failed=$true}
if(-not $failed){throw 'Live drift accepted'};$checks++
Write-Output "PASS $checks lease preflight scenarios; fake process/installation, no wait or restore"
