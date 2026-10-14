[CmdletBinding(SupportsShouldProcess)]
param([ValidateSet('Preflight','Install','Restore')][string]$Action='Preflight',
 [Parameter(Mandatory)][string]$ManifestPath,
 [string]$GameRoot='C:\Program Files (x86)\Steam\steamapps\common\Singularity',
 [string]$BackupPath='')
# MAIN only. Distinct delta lease; never overlaps the existing four-file or WAV leases.
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Utility')
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Management')
function Closed {
 if(-not(Get-CimInstance Win32_Process -Filter "ProcessId=$PID")){throw 'Full process view required'}
 if(@(Get-Process -Name Singularity -ErrorAction SilentlyContinue).Count){throw 'Game running; no writes'}
}
function Hash([string]$p){if(Test-Path -LiteralPath $p -PathType Leaf){return (Get-FileHash -LiteralPath $p).Hash};return 'ABSENT'}
function Child([string]$root,[string]$relative){
 $base=[IO.Path]::GetFullPath($root).TrimEnd('\','/')
 $path=[IO.Path]::GetFullPath((Join-Path $base $relative))
 if(-not $path.StartsWith($base+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Path escape'}
 $cursor=$path
 while($cursor.Length -ge $base.Length){
  if((Test-Path -LiteralPath $cursor) -and ((Get-Item -LiteralPath $cursor).Attributes -band [IO.FileAttributes]::ReparsePoint)){throw 'Reparse path refused'}
  $next=Split-Path $cursor;if($next -eq $cursor){break};$cursor=$next
 }
 return $path
}
$game=[IO.Path]::GetFullPath($GameRoot)
$manifestFull=(Resolve-Path -LiteralPath $ManifestPath).Path
$sourceRoot=Split-Path $manifestFull
$manifest=Get-Content $manifestFull -Raw | ConvertFrom-Json
$dds=@('AR_E99Rifle_alt_d','AR_E99Rifle_n','AR_shotgun_d','AR_shotgun_n','ar_gatling_gun_d','ar_gatling_gun_n',
 'laser_d','laser_n','revert_d2','revert_n2','phasetick_d','phasetick_n','Spetsnaz_MP_helmet_d','Spetsnaz_MP_helmet_n',
 'Spetsnaz_MP_Body_d','Spetsnaz_MP_Body_n','door_rollingT_d','door_rollingT_n','door_blastdoorN_d','door_blastdoorN_n',
 'inview_glove_d','inview_glove_n','elite_beam_n','ar_sniper_rifle_n')
$allowed=@('RvGame\CookedPC\UI_SinglePlayer_SF.xxx','RvGame\CookedPC\SP_VI2_kismet01_LOC_INT.xxx',
 'RvGame\CookedPC\SP_VI2_kismet01_a_LOC_INT.xxx','RvGame\CookedPC\SP_VI2_kismet01_b_LOC_INT.xxx',
 'Binaries\texture_override.ini','Binaries\vr-assets\localization\pl\polish_text.jsonl',
 'Binaries\vr-assets\localization\pl\polish_voice.jsonl','Binaries\vr-assets\localization\pl\polish_story_voice.jsonl')+
 @($dds|ForEach-Object {'Binaries\texture-override-remaster\'+$_+'.dds'})
$expectedCount=32
if($manifest.Kind -cin @('SingularityVR post-opening asset delta v2',
 'SingularityVR first-area polish delta v3','SingularityVR first-area font polish delta v4')){
 $allowed+=@('RvGame\CookedPC\SP_VI2_zone02a_LOC_INT.xxx','RvGame\CookedPC\SP_VI2_zone02a_2010_LOC_INT.xxx')
 $expectedCount=34
 if($manifest.Kind -cne 'SingularityVR post-opening asset delta v2'){
  $allowed+=@('RvGame\CookedPC\SP_VI2_kismet02_LOC_INT.xxx','RvGame\CookedPC\SP_VI2_kismet03_LOC_INT.xxx',
              'RvGame\CookedPC\SP_VI2_kismet04_LOC_INT.xxx')
  $expectedCount=37
  if($manifest.Kind -ceq 'SingularityVR first-area font polish delta v4'){
   $allowed+=@('RvGame\CookedPC\ui_GFx_fonts.xxx');$expectedCount=38
  }
 }
}elseif($manifest.Kind -cne 'SingularityVR prepared asset delta v1'){
 if($manifest.Kind -ceq 'SingularityVR xlingual bank delta v5'){
  $allowed=@('RvGame\CookedPC\SP_VI2_kismet01_LOC_INT.xxx',
   'RvGame\CookedPC\SP_VI2_kismet01_a_LOC_INT.xxx','RvGame\CookedPC\SP_VI2_kismet01_b_LOC_INT.xxx',
   'RvGame\CookedPC\SP_VI2_kismet02_LOC_INT.xxx','RvGame\CookedPC\SP_VI2_kismet03_LOC_INT.xxx',
   'RvGame\CookedPC\SP_VI2_kismet04_LOC_INT.xxx','RvGame\CookedPC\SP_VI2_zone02a_LOC_INT.xxx',
   'RvGame\CookedPC\SP_VI2_zone02a_2010_LOC_INT.xxx')
  $expectedCount=8
 }elseif($manifest.Kind -ceq 'SingularityVR intro bank delta v6'){
  $allowed=@('RvGame\CookedPC\SP_VI_intro_Kismet_LOC_INT.xxx')
  $expectedCount=1
 }else{throw 'Unknown exact delta profile'}
}
if(@($manifest.Files).Count -ne $expectedCount -or
 (($manifest.Files|ForEach-Object {$_.RelativePath}|Sort-Object) -join '|') -cne (($allowed|Sort-Object) -join '|')){throw 'Exact32/34/37/38 delta targets required'}
Closed
foreach($f in $manifest.Files){
 if($f.BaselineSha256 -cnotmatch '^([A-F0-9]{64}|ABSENT)$' -or $f.CandidateSha256 -cnotmatch '^[A-F0-9]{64}$'){throw 'Invalid hash'}
 $null=Child $game $f.RelativePath
}
function CopyExact([string]$source,[string]$target,[string]$expected){
 Closed;if((Hash $source) -cne $expected){throw 'Changed source'}
 if(-not(Test-Path -LiteralPath (Split-Path $target))){New-Item -ItemType Directory -Path (Split-Path $target) -Force | Out-Null}
 $temp=$target+".prepared-$PID.new"
 if(Test-Path -LiteralPath $temp){throw 'Existing copy temporary file'}
 try{
  Copy-Item -LiteralPath $source -Destination $temp
  if((Hash $temp) -cne $expected){throw 'Temporary copy mismatch'}
  Closed;Move-Item -LiteralPath $temp -Destination $target -Force
  if((Hash $target) -cne $expected){throw 'Installed hash mismatch'}
 }finally{if(Test-Path -LiteralPath $temp){Remove-Item -LiteralPath $temp}}
}
if($Action -eq 'Restore'){
 if(-not $BackupPath){throw 'Exact backup identity required'}
 $backup=(Resolve-Path -LiteralPath $BackupPath).Path
 $record=Get-Content (Join-Path $backup 'lease.json') -Raw | ConvertFrom-Json
 if($record.GameRoot -cne $game -or $record.ManifestSha256 -cne (Hash $manifestFull)){throw 'Wrong backup/manifest'}
 foreach($f in $manifest.Files){if($f.BaselineSha256 -cne 'ABSENT' -and (Hash (Child $backup $f.RelativePath)) -cne $f.BaselineSha256){throw 'Bad backup; zero restore writes'}}
 if(-not $PSCmdlet.ShouldProcess($game,"Restore exact$expectedCount owned asset files")){return}
 $conflicts=@()
 foreach($f in $manifest.Files){
  $target=Child $game $f.RelativePath;$current=Hash $target
  if($current -ceq $f.BaselineSha256){continue}
  if($current -cne $f.CandidateSha256){$conflicts+=$f.RelativePath;continue}
  if($f.BaselineSha256 -ceq 'ABSENT'){Closed;Remove-Item -LiteralPath $target}
  else{CopyExact (Child $backup $f.RelativePath) $target $f.BaselineSha256}
 }
 if($conflicts.Count){throw ('Unexpected user changes preserved: '+($conflicts -join ', '))}
 foreach($f in $manifest.Files){if((Hash (Child $game $f.RelativePath)) -cne $f.BaselineSha256){throw 'Final restore mismatch'}}
 Write-Output "RESTORED:$expectedCount exact asset baselines; newly created owned files removed";return
}
foreach($f in $manifest.Files){
 if((Hash (Child $sourceRoot $f.Source)) -cne $f.CandidateSha256){throw 'Candidate changed'}
 if((Hash (Child $game $f.RelativePath)) -cne $f.BaselineSha256){throw ('Baseline changed: '+$f.RelativePath)}
}
Write-Output "PREFLIGHT PASS: exact$expectedCount independent delta files, no core-DLL/INI/native-intro/WAV overlap"
if($Action -eq 'Preflight' -or $WhatIfPreference){return}
if(-not $PSCmdlet.ShouldProcess($game,"Install exact$expectedCount prepared asset test files")){return}
$repo=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$backup=Join-Path $repo ('deployment\hands-dubbing\asset-backups\'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'-'+$PID)
New-Item -ItemType Directory -Path $backup | Out-Null
foreach($f in $manifest.Files){
 if($f.BaselineSha256 -ceq 'ABSENT'){continue}
 $dest=Child $backup $f.RelativePath;New-Item -ItemType Directory -Path (Split-Path $dest) -Force | Out-Null
 Copy-Item -LiteralPath (Child $game $f.RelativePath) -Destination $dest
 if((Hash $dest) -cne $f.BaselineSha256){throw 'Backup mismatch before writes'}
}
@{GameRoot=$game;ManifestSha256=(Hash $manifestFull);Created=(Get-Date).ToString('o')} | ConvertTo-Json | Set-Content (Join-Path $backup 'lease.json') -Encoding UTF8
try{
 foreach($f in $manifest.Files){
  $target=Child $game $f.RelativePath
  if((Hash $target) -cne $f.BaselineSha256){throw 'Baseline changed during backup/install; preserve unowned write'}
  CopyExact (Child $sourceRoot $f.Source) $target $f.CandidateSha256
 }
}catch{
 Write-Error -ErrorAction Continue $_
 & $PSCommandPath -Action Restore -ManifestPath $manifestFull -GameRoot $game -BackupPath $backup
 throw 'Delta installation failed; owned rollback attempted'
}
Write-Output ('ASSET BACKUP: '+$backup)
Write-Output "INSTALLED:$expectedCount candidate hashes verified; Steam launch is separate and MAIN-only"
