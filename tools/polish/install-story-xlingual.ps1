[CmdletBinding(SupportsShouldProcess)]
param([ValidateSet('Preflight','Install','Rollback')][string]$Action='Preflight',
 [string]$SourceDirectory='C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\polish\story_voicepack_xlingual',
 [string]$GameRoot='C:\Program Files (x86)\Steam\steamapps\common\Singularity',
 [Parameter(Mandatory)][string]$EvidenceDirectory)
# Persistent user-requested STORY-only deployment. Never launch or modify banks,
# intro, UI894, multiplayer, renderer, manifests or configuration.
Set-StrictMode -Version Latest;$ErrorActionPreference='Stop'
function Closed {
 if(-not(Get-CimInstance Win32_Process -Filter "ProcessId=$PID")){throw 'Full process context required'}
 if(@(Get-CimInstance Win32_Process -Filter "Name='Singularity.exe'").Count){throw 'Game running; no writes'}
}
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash}
function Root([string]$p){
 $full=(Resolve-Path -LiteralPath $p).Path
 if((Get-Item -LiteralPath $full).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Reparse root refused'}
 return $full
}
function Wave([string]$p){
 $b=[IO.File]::ReadAllBytes($p)
 if($b.Length -lt 44 -or $b.Length -gt 16777216 -or [Text.Encoding]::ASCII.GetString($b,0,4) -cne 'RIFF' -or
  [Text.Encoding]::ASCII.GetString($b,8,4) -cne 'WAVE' -or [BitConverter]::ToUInt32($b,4)+8 -ne $b.Length){throw "Invalid WAV: $p"}
 $pos=12;$fmt=$false;$data=$false
 while($pos+8 -le $b.Length){
  $id=[Text.Encoding]::ASCII.GetString($b,$pos,4);$n=[long][BitConverter]::ToUInt32($b,$pos+4);$at=$pos+8
  if($at+$n -gt $b.Length){throw 'Truncated WAV'}
  if($id -ceq 'fmt '){
   if($n -lt 16 -or [BitConverter]::ToUInt16($b,$at) -ne 1 -or [BitConverter]::ToUInt16($b,$at+2) -ne 1 -or
    [BitConverter]::ToUInt32($b,$at+4) -ne 24000 -or [BitConverter]::ToUInt16($b,$at+12) -ne 2 -or
    [BitConverter]::ToUInt16($b,$at+14) -ne 16){throw 'Expected mono PCM16 24kHz'};$fmt=$true
  }
  if($id -ceq 'data'){if($n -le 0 -or $n%2){throw 'Invalid PCM payload'};$data=$true}
  $pos=$at+$n+($n%2)
 }
 if(-not $fmt -or -not $data -or $pos -ne $b.Length){throw 'Invalid WAV chunks'}
}
Closed
$game=Root $GameRoot;$target=Root (Join-Path $game 'Binaries/vr-assets/audio/polish/story')
$evidence=[IO.Path]::GetFullPath($EvidenceDirectory)
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
if(-not $evidence.StartsWith($repo+'\deployment\',[StringComparison]::OrdinalIgnoreCase)){throw 'Evidence must be private in this worktree'}
function CopyExact([string]$source,[string]$dest,[string]$expected){
 if(@(Get-Process -Name Singularity -ErrorAction SilentlyContinue).Count){throw 'Game running; no copy'}
 if((Hash $source) -cne $expected){throw 'Source changed'}
 $temporary=$dest+".story-$PID.new"
 if(Test-Path -LiteralPath $temporary){throw 'Existing temporary file'}
 try{
  Copy-Item -LiteralPath $source -Destination $temporary
  if((Hash $temporary) -cne $expected){throw 'Staged hash mismatch'}
  Closed;Move-Item -LiteralPath $temporary -Destination $dest -Force
  if((Hash $dest) -cne $expected){throw 'Installed hash mismatch'}
 }finally{if(Test-Path -LiteralPath $temporary){Remove-Item -LiteralPath $temporary}}
}
$receipt=Join-Path $evidence 'story-install.json'
if($Action -eq 'Rollback'){
 $record=Get-Content -LiteralPath $receipt -Raw | ConvertFrom-Json
 if($record.Kind -cne 'SingularityVR story-only xlingual v1' -or $record.GameRoot -cne $game -or @($record.Files).Count -ne 992){throw 'Wrong rollback identity'}
 $seen=@{}
 foreach($f in $record.Files){
  if($f.Name -cnotmatch '^[a-f0-9]{16}\.wav$' -or $seen.ContainsKey($f.Name)){throw 'Unsafe/duplicate WAV identity'};$seen[$f.Name]=$true
  if((Hash (Join-Path $evidence ('backup/'+$f.Name))) -cne $f.BaselineSha256){throw 'Bad backup; zero rollback writes'}
 }
 if(-not $PSCmdlet.ShouldProcess($target,'Restore exactly992 story WAVs')){return}
 $conflicts=@()
 foreach($f in $record.Files){
  $dest=Join-Path $target $f.Name;$current=Hash $dest
  if($current -ceq $f.BaselineSha256){continue}
  if($current -cne $f.CandidateSha256){$conflicts+=$f.Name;continue}
  CopyExact (Join-Path $evidence ('backup/'+$f.Name)) $dest $f.BaselineSha256
 }
 if($conflicts.Count){throw 'Unexpected story changes retained'}
 Write-Output 'RESTORED:992 story original hashes';return
}
$source=Root $SourceDirectory
$rows=@(Get-Content -LiteralPath (Join-Path $source 'manifest_xlingual.jsonl') | Where-Object {$_} | ForEach-Object {$_|ConvertFrom-Json})
$files=@(Get-ChildItem -LiteralPath $source -Filter '*.wav' -File)
$installed=@(Get-ChildItem -LiteralPath $target -Filter '*.wav' -File)
if($rows.Count -ne 992 -or $files.Count -ne 992 -or $installed.Count -ne 992){throw 'Exactly992 source/manifest/installed story files required'}
if((($files.Name|Sort-Object)-join '|') -cne (($installed.Name|Sort-Object)-join '|')){throw 'Story is not a drop-in filename set'}
$names=@{};$records=@()
foreach($r in $rows){
 if($r.file -cnotmatch '^[a-f0-9]{16}\.wav$' -or $r.package -notmatch '^SP_' -or $names.ContainsKey($r.file)){throw 'Unsafe/duplicate/non-SP source entry'}
 $names[$r.file]=$true;$path=Join-Path $source $r.file;$dest=Join-Path $target $r.file
 foreach($p in @($path,$dest)){if((Get-Item -LiteralPath $p).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Reparse file refused'}}
 Wave $path
 $records+=@{Name=$r.file;CandidateSha256=(Hash $path);BaselineSha256=(Hash $dest)}
}
# Snapshot protected UI voices. These hashes are verified after installation;
# no UI voice, intro, native bank or game configuration is a write target.
$ui=Root (Join-Path $game 'Binaries/vr-assets/audio/polish/ui')
$protected=@(Get-ChildItem -LiteralPath $ui -Filter '*.wav' -File | ForEach-Object {@{Path=$_.FullName;Sha256=(Hash $_.FullName)}})
if($protected.Count -ne 894){throw 'Expected894 protected UI WAVs'}
Write-Output 'PREFLIGHT PASS:992 mono PCM16 24kHz SP voices; exact lookup filenames;894 UI voices protected'
if($Action -eq 'Preflight' -or $WhatIfPreference){return}
if(Test-Path -LiteralPath $evidence){throw 'Existing evidence refused'}
if(-not $PSCmdlet.ShouldProcess($target,'Install exactly992 requested story WAVs, with all original backups')){return}
New-Item -ItemType Directory -Path (Join-Path $evidence 'backup') | Out-Null
foreach($f in $records){
 $dest=Join-Path $target $f.Name
 if((Hash $dest) -cne $f.BaselineSha256){throw 'Story baseline changed before backup'}
 $backup=Join-Path $evidence ('backup/'+$f.Name);Copy-Item -LiteralPath $dest -Destination $backup
 if((Hash $backup) -cne $f.BaselineSha256){throw 'Backup mismatch'}
}
@{Kind='SingularityVR story-only xlingual v1';CreatedAt=(Get-Date).ToString('o');GameRoot=$game;
 SourceDirectory=$source;Files=$records;ProtectedUi=$protected;AuditoryAcceptance='PENDING;825b mid-sentence QA pending';
 NativeBankReplacement=$false} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $receipt -Encoding UTF8
try{
 foreach($f in $records){
  $dest=Join-Path $target $f.Name
  if((Hash $dest) -cne $f.BaselineSha256){throw 'Story baseline changed during install'}
  CopyExact (Join-Path $source $f.Name) $dest $f.CandidateSha256
 }
 foreach($f in $records){if((Hash (Join-Path $target $f.Name)) -cne $f.CandidateSha256){throw 'Final story mismatch'}}
 foreach($p in $protected){if((Hash $p.Path) -cne $p.Sha256){throw 'Protected UI voice changed'}}
}catch{& $PSCommandPath -Action Rollback -GameRoot $game -EvidenceDirectory $evidence;throw}
Write-Output "INSTALLED:992 verified story WAVs;894 UI voices unchanged; backup $evidence"
Write-Output 'Native UE3 bank audio is a different path; this copy is not proof of full-game Polish playback'
