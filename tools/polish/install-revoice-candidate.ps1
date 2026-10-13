[CmdletBinding(SupportsShouldProcess)]
param(
    [ValidateSet('Plan','Install','Rollback')][string]$Action='Plan',
    [string]$CandidateDirectory,
    [string]$GameRoot='C:\Program Files (x86)\Steam\steamapps\common\Singularity',
    [Parameter(Mandatory)][string]$EvidenceDirectory,
    [switch]$AllowPendingQa
)
# MAIN only. WAV-only replacement, keeping the installed lookup filenames.
# Does NOT hook/mute native UE3 voices, change manifests, INI, DLLs or Bink files.
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$script:ProcessVisibilityChecked=$false
function Assert-Closed {
    if(-not $script:ProcessVisibilityChecked){
        if(-not(Get-CimInstance Win32_Process -Filter "ProcessId=$PID")){throw 'Full process visibility required'}
        if(@(Get-CimInstance Win32_Process -Filter "Name='Singularity.exe'").Count){throw 'Game running; no WAV writes'}
        $script:ProcessVisibilityChecked=$true
    }
    if(@(Get-Process -Name Singularity -ErrorAction SilentlyContinue).Count){throw 'Game running; no WAV writes'}
}
function Hash([string]$Path){(Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash}
function Resolve-Child([string]$Root,[string]$Relative){
    $base=[IO.Path]::GetFullPath($Root).TrimEnd('\','/')+[IO.Path]::DirectorySeparatorChar
    $path=[IO.Path]::GetFullPath((Join-Path $Root $Relative))
    if(-not $path.StartsWith($base,[StringComparison]::OrdinalIgnoreCase)){throw 'Path escapes owned directory'}
    if((Get-Item -LiteralPath $Root).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Reparse root rejected'}
    $parent=Split-Path $path
    while($parent.Length -ge $base.TrimEnd('\').Length){
        if((Test-Path -LiteralPath $parent) -and ((Get-Item -LiteralPath $parent).Attributes -band [IO.FileAttributes]::ReparsePoint)){throw 'Reparse parent rejected'}
        $next=Split-Path $parent
        if($next -eq $parent){break};$parent=$next
    }
    if((Test-Path -LiteralPath $path) -and ((Get-Item -LiteralPath $path).Attributes -band [IO.FileAttributes]::ReparsePoint)){throw 'Reparse file rejected'}
    return $path
}
function Assert-Wave([string]$Path){
    $b=[IO.File]::ReadAllBytes($Path)
    if($b.Length -lt 44 -or [Text.Encoding]::ASCII.GetString($b,0,4) -ne 'RIFF' -or [Text.Encoding]::ASCII.GetString($b,8,4) -ne 'WAVE'){throw 'Invalid RIFF WAVE'}
    if([BitConverter]::ToUInt32($b,4)+8 -ne $b.Length){throw 'Invalid RIFF length'}
    $pos=12;$fmt=$false;$data=$false
    while($pos+8 -le $b.Length){
        $id=[Text.Encoding]::ASCII.GetString($b,$pos,4);$n=[long][BitConverter]::ToUInt32($b,$pos+4);$p=$pos+8
        if($p+$n -gt $b.Length){throw 'Truncated WAV chunk'}
        if($id -eq 'fmt '){
            if($n -lt 16 -or [BitConverter]::ToUInt16($b,$p) -ne 1 -or [BitConverter]::ToUInt16($b,$p+2) -ne 1 -or [BitConverter]::ToUInt32($b,$p+4) -ne 24000 -or [BitConverter]::ToUInt16($b,$p+12) -ne 2 -or [BitConverter]::ToUInt16($b,$p+14) -ne 16){throw 'Expected mono PCM16 24kHz'}
            $fmt=$true
        }
        if($id -eq 'data'){if($n -le 0 -or $n%2){throw 'Invalid WAV data'};$data=$true}
        $pos=$p+$n+($n%2)
    }
    if(-not $fmt -or -not $data -or $pos -ne $b.Length){throw 'Invalid WAV chunks'}
}
function Copy-Verified([string]$Source,[string]$Destination,[string]$Expected){
    Assert-Closed
    if((Hash $Source) -cne $Expected){throw 'Source changed before copy'}
    Copy-Item -LiteralPath $Source -Destination $Destination -Force
    if((Hash $Destination) -cne $Expected){throw 'Destination copy mismatch'}
}
$game=(Resolve-Path -LiteralPath $GameRoot).Path
$evidence=[IO.Path]::GetFullPath($EvidenceDirectory)
$planPath=Join-Path $evidence 'revoice-lease.json'
Assert-Closed
if($Action -eq 'Rollback'){
    $plan=Get-Content -LiteralPath $planPath -Raw | ConvertFrom-Json
    if($plan.Kind -cne 'SingularityVR revoice WAV lease v1' -or $plan.GameRoot -ine $game -or @($plan.Files).Count -ne 1886){throw 'Wrong rollback plan'}
    $unique=@{}
    foreach($f in $plan.Files){
        if($f.RelativePath -notmatch '^Binaries\\vr-assets\\audio\\polish\\(ui|story)\\[a-zA-Z0-9_-]+\.wav$' -or $f.Backup -notmatch '^backup\\(ui|story)\\[a-zA-Z0-9_-]+\.wav$' -or $f.BaselineSha256 -cnotmatch '^[A-F0-9]{64}$' -or $f.CandidateSha256 -cnotmatch '^[A-F0-9]{64}$'){throw 'Invalid WAV-only restore scope'}
        if($unique.ContainsKey($f.RelativePath)){throw 'Duplicate restore target'};$unique[$f.RelativePath]=$true
        $null=Resolve-Child $game $f.RelativePath
        $b=Resolve-Child $evidence $f.Backup
        if((Hash $b) -cne $f.BaselineSha256){throw 'Backup invalid; no restore writes'}
    }
    if(-not $PSCmdlet.ShouldProcess($game,'Restore the 1886 owned WAV files')){return}
    $conflicts=@()
    foreach($f in $plan.Files){
        $target=Resolve-Child $game $f.RelativePath
        $actual=Hash $target
        if($actual -ceq $f.BaselineSha256){continue}
        if($actual -cne $f.CandidateSha256){$conflicts+=$f.RelativePath;continue}
        Copy-Verified (Resolve-Child $evidence $f.Backup) $target $f.BaselineSha256
    }
    if($conflicts.Count){throw ('Preserved unexpected WAV changes: '+$conflicts.Count)}
    Write-Output 'RESTORED: 1886 exact original WAV hashes';return
}
if(Test-Path -LiteralPath $evidence){throw 'Refuse existing evidence directory'}
$src=(Resolve-Path -LiteralPath $CandidateDirectory).Path
$manifest=Resolve-Child $src 'manifest.jsonl';$qaPath=Resolve-Child $src 'qa.json'
$manifestHash=Hash $manifest;$qaHash=Hash $qaPath
$rows=@(Get-Content -LiteralPath $manifest | Where-Object {$_} | ForEach-Object {$_|ConvertFrom-Json})
$qa=@(Get-Content -LiteralPath $qaPath -Raw | ConvertFrom-Json)
if($rows.Count -ne 1886 -or $qa.Count -ne 1886){throw 'Expected exactly 1886 entries'}
$pending=@($qa | Where-Object {$_.qa_status -cne 'PASS_AUTOMATED_SCREEN'}).Count
if($pending -and -not $AllowPendingQa){throw 'Pending QA requires explicit AllowPendingQa'}
$ids=@{};$qaIds=@{};$targets=@{};$counts=@{full_localization=0;story_nllb=0};$files=@()
foreach($q in $qa){if($qaIds.ContainsKey($q.line_id)){throw 'Duplicate QA identity'};$qaIds[$q.line_id]=$q}
$lookupDir=Join-Path $game 'Binaries\vr-assets\localization\pl'
$ui=@{};$story=@{}
foreach($x in @(Get-Content (Join-Path $lookupDir 'polish_voice.jsonl') | ForEach-Object {$_|ConvertFrom-Json})){$ui[$x.id]=$x.file}
foreach($x in @(Get-Content (Join-Path $lookupDir 'polish_story_voice.jsonl') | ForEach-Object {$_|ConvertFrom-Json})){$story[$x.source]=$x.file}
foreach($r in $rows){
    if($ids.ContainsKey($r.line_id) -or -not $qaIds.ContainsKey($r.line_id)){throw 'Duplicate or missing identity'};$ids[$r.line_id]=$true
    if(-not $counts.ContainsKey($r.pack)){throw 'Unknown pack'};$counts[$r.pack]++
    $name=[IO.Path]::GetFileName($r.previous_audio)
    if($name -notmatch '^[a-zA-Z0-9_-]+\.wav$'){throw 'Unsafe lookup filename'}
    $map=if($r.pack -eq 'full_localization'){$ui}else{$story}
    if(-not $map.ContainsKey($r.line_id) -or $map[$r.line_id] -cne $name){throw 'Installed lookup identity mismatch'}
    $group=if($r.pack -eq 'full_localization'){'ui'}else{'story'}
    $relative='Binaries\vr-assets\audio\polish\'+$group+'\'+$name
    if($targets.ContainsKey($relative)){throw 'Duplicate target'};$targets[$relative]=$true
    $target=Resolve-Child $game $relative
    $source=Resolve-Child $src ([IO.Path]::GetFileName($r.file))
    if($source -ine [IO.Path]::GetFullPath($r.file) -or $qaIds[$r.line_id].file -ine $source){throw 'Candidate/QA source mismatch'}
    Assert-Wave $source
    $files += [pscustomobject]@{RelativePath=$relative;Source=$source;CandidateSha256=(Hash $source);BaselineSha256=(Hash $target);Backup=('backup\'+$group+'\'+$name)}
}
if($counts.full_localization -ne 894 -or $counts.story_nllb -ne 992){throw 'Pack counts mismatch'}
if((Hash $manifest) -cne $manifestHash -or (Hash $qaPath) -cne $qaHash){throw 'Candidate metadata changed during validation'}
$plan=[ordered]@{Kind='SingularityVR revoice WAV lease v1';GameRoot=$game;CreatedAt=(Get-Date).ToString('o');CandidateManifestSha256=$manifestHash;QaSha256=$qaHash;PendingQa=$pending;NativeGameplayReplacement=$false;Files=$files}
if(-not $PSCmdlet.ShouldProcess($evidence,($Action+' exact WAV-only plan (1886 files)'))){return}
New-Item -ItemType Directory -Path $evidence | Out-Null
$plan | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $planPath -Encoding UTF8
Write-Output ('PLAN PASS: 894 UI + 992 story; pending QA='+$pending+'; all WAV formats and lookup identities verified')
if($Action -eq 'Plan'){return}
Assert-Closed
foreach($f in $files){
    $backup=Join-Path $evidence $f.Backup
    New-Item -ItemType Directory -Path (Split-Path $backup) -Force | Out-Null
    Copy-Verified (Resolve-Child $game $f.RelativePath) $backup $f.BaselineSha256
}
# All backups and sources validated before any installed WAV is replaced.
foreach($f in $files){if((Hash $f.Source) -cne $f.CandidateSha256){throw 'Candidate changed; no install writes'}}
try{
    foreach($f in $files){
        $target=Resolve-Child $game $f.RelativePath
        if((Hash $target) -cne $f.BaselineSha256){throw 'Installed WAV changed during lease'}
        Copy-Verified $f.Source $target $f.CandidateSha256
    }
}catch{
    $failure=$_
    & $PSCommandPath -Action Rollback -GameRoot $game -EvidenceDirectory $evidence
    throw $failure
}
Write-Output 'INSTALLED: all 1886 candidate WAV SHA-256 hashes verified; native UE3 audio is not redirected by this operation'
