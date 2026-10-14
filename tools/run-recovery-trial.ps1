[CmdletBinding()]
param([Parameter(Mandatory)][string]$LiveDirectory)
# MAIN only; user start required. Exact manifests, independent guards, real Steam.
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$repo=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$live=[IO.Path]::GetFullPath($LiveDirectory)
if(-not $live.StartsWith($repo+'\deployment\hands-dubbing\',[StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $live)){throw 'Fresh owned live directory required'}
$game='C:\Program Files (x86)\Steam\steamapps\common\Singularity'
$ready=Join-Path $repo 'deployment/hands-dubbing/recovery-ready'
$core=Join-Path $ready 'core/test-manifest.json';$assets=Join-Path $ready 'assets/asset-manifest.json'
$accepted='C:\Users\Trilex\vr-mods\SingularityVR-post-opening\deployment\hands-dubbing\post-opening'
$voicePlanPath=Join-Path $accepted 'next-quiet-plan-v1/revoice-lease.json'
function Hash([string]$p){if(Test-Path -LiteralPath $p -PathType Leaf){return (Get-FileHash -LiteralPath $p).Hash};return 'ABSENT'}
if(-not(Get-CimInstance Win32_Process -Filter "ProcessId=$PID")){throw 'Full process visibility required'}
if(@(Get-CimInstance Win32_Process -Filter "Name='Singularity.exe'").Count){throw 'Game running; no deployment'}
if((Hash $voicePlanPath) -cne '68569A3139E9D5D8E53803A73ECBAC6650061BF066B220818D619B2C3873859F'){throw 'Accepted voice plan changed'}
$build=Get-Content (Join-Path $ready 'd3d9.build.json') -Raw | ConvertFrom-Json
if($build.commit -cne 'c51f53a8e814af0f1741f5d89bd2279fad352393' -or
 $build.sha256 -cne 'EA9EFFB10C83815DAA36131F1F7785D2CC1B83C55866DF84AD0700CC335FD71C' -or
 $build.architecture -cne 'x86 PE32' -or $build.optimization -cne '/O2'){throw 'Build provenance changed'}
$voicePlan=Get-Content $voicePlanPath -Raw | ConvertFrom-Json
foreach($f in $voicePlan.Files){
 if((Hash (Join-Path $game $f.RelativePath)) -cne $f.BaselineSha256 -or (Hash $f.Source) -cne $f.CandidateSha256){throw 'Voice baseline/candidate changed'}
}
& (Join-Path $repo 'tools/deploy-singularityvr.ps1') -Action Status -OpeningCutsceneManifestPath $core
& (Join-Path $repo 'tools/test-prepared-assets.ps1') -Action Preflight -ManifestPath $assets
New-Item -ItemType Directory -Path $live | Out-Null
$coreBackup='';$assetBackup='';$voicesInstalled=$false;$voiceLive=Join-Path $live 'revoice'
$guardPids=@{}
function Guard([string]$name,[string]$script,[string[]]$guardArguments){
 $out=Join-Path $live ($name+'.stdout.log');$err=Join-Path $live ($name+'.stderr.log')
 $argv=@('-NoProfile','-ExecutionPolicy','Bypass','-File',('"'+$script+'"'))+$guardArguments
 $proc=Start-Process -FilePath "$env:SystemRoot\System32\WindowsPowerShell\v1.0\powershell.exe" -ArgumentList $argv -WindowStyle Hidden -PassThru -RedirectStandardOutput $out -RedirectStandardError $err
 $null=$proc.Handle;$until=(Get-Date).AddSeconds(45)
 while((Get-Date) -lt $until){
  if((Test-Path -LiteralPath $out) -and (Select-String -LiteralPath $out -Pattern '^READY:|^OBSERVER READY:')){
   $guardPids[$name]=$proc.Id;Write-Output "READY: $name PID $($proc.Id)";return
  }
  $proc.Refresh();if($proc.HasExited){throw "Guard $name exited before READY: $err"}
  Start-Sleep -Milliseconds 250
 }
 throw "Guard $name did not arm"
}
try{
 $result=@(& (Join-Path $repo 'tools/deploy-singularityvr.ps1') -Action Install -OpeningCutsceneManifestPath $core)
 $result | Tee-Object -FilePath (Join-Path $live 'core-install.log') | Write-Output
 $coreBackup=(@($result|Where-Object {$_ -is [string] -and $_.StartsWith('BACKUP: ')})[0]).Substring(8)
 Guard 'core-guard' (Join-Path $repo 'tools/watch-opening-cutscene-restore.ps1') @('-ManifestPath',('"'+$core+'"'),'-BackupPath',('"'+$coreBackup+'"'),'-LaunchWindowSeconds','600')
 $result=@(& (Join-Path $repo 'tools/test-prepared-assets.ps1') -Action Install -ManifestPath $assets)
 $result | Tee-Object -FilePath (Join-Path $live 'asset-install.log') | Write-Output
 $assetBackup=(@($result|Where-Object {$_ -is [string] -and $_.StartsWith('ASSET BACKUP: ')})[0]).Substring(14)
 Guard 'asset-guard' (Join-Path $repo 'tools/watch-prepared-assets-restore.ps1') @('-ManifestPath',('"'+$assets+'"'),'-BackupPath',('"'+$assetBackup+'"'),'-LaunchWindowSeconds','600')
 & (Join-Path $repo 'tools/polish/install-revoice-candidate.ps1') -Action Install -CandidateDirectory (Join-Path $accepted 'quiet-revoice-v2') -EvidenceDirectory $voiceLive -AllowPendingQa | Tee-Object -FilePath (Join-Path $live 'voice-install.log') | Write-Output
 $voicesInstalled=$true
 Guard 'voice-guard' (Join-Path $repo 'tools/polish/watch-revoice-restore.ps1') @('-EvidenceDirectory',('"'+$voiceLive+'"'),'-LaunchWindowSeconds','600')
 $checked=0
 foreach($m in @($core,$assets)){
  foreach($f in (Get-Content $m -Raw | ConvertFrom-Json).Files){if((Hash (Join-Path $game $f.RelativePath)) -cne $f.CandidateSha256){throw 'Installed candidate mismatch'};$checked++}
 }
 foreach($f in (Get-Content (Join-Path $voiceLive 'revoice-lease.json') -Raw | ConvertFrom-Json).Files){if((Hash (Join-Path $game $f.RelativePath)) -cne $f.CandidateSha256){throw 'Installed WAV mismatch'};$checked++}
 if($checked -ne 1928 -or (Hash (Join-Path $game 'Binaries/d3d9.dll')) -cne $build.sha256){throw 'Final identity gate failed'}
 Guard 'observer' (Join-Path $repo 'tools/watch-cutscene-session.ps1') @('-EvidenceDirectory',('"'+$live+'"'),'-LaunchWindowSeconds','600','-ObserveSeconds','1200')
 $record=[ordered]@{CreatedAt=(Get-Date).ToString('o');CandidateDllSha256=$build.sha256;RuntimeSourceCommit=$build.commit;
  CoreManifest=$core;AssetManifest=$assets;CoreBackup=$coreBackup;AssetBackup=$assetBackup;VoiceEvidence=$voiceLive;
  GuardPids=$guardPids;VerifiedFiles=$checked;Launched=$false}
 $record | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $live 'live.json') -Encoding UTF8
 if(@(Get-CimInstance Win32_Process -Filter "Name='Singularity.exe'").Count){throw 'Unexpected game process before controlled launch'}
 Start-Process -FilePath 'C:\Program Files (x86)\Steam\steam.exe' -ArgumentList @('-applaunch','42670') -WindowStyle Hidden
 $record.Launched=$true
 $record | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $live 'live.json') -Encoding UTF8
 Write-Output "STEAM LAUNCH REQUESTED: $checked hashes and three backup guards; $live"
}catch{
 $failure=$_
 if(-not @(Get-CimInstance Win32_Process -Filter "Name='Singularity.exe'").Count){
  if($voicesInstalled){& (Join-Path $repo 'tools/polish/install-revoice-candidate.ps1') -Action Rollback -EvidenceDirectory $voiceLive}
  if($assetBackup){& (Join-Path $repo 'tools/test-prepared-assets.ps1') -Action Restore -ManifestPath $assets -BackupPath $assetBackup}
  if($coreBackup){& (Join-Path $repo 'tools/deploy-singularityvr.ps1') -Action Rollback -OpeningCutsceneManifestPath $core -BackupPath $coreBackup}
 }
 throw $failure
}
