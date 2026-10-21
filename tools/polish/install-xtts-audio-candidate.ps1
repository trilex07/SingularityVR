[CmdletBinding(SupportsShouldProcess)]
param(
    [ValidateSet('Plan','Install','Rollback')][string]$Action='Plan',
    [Parameter(Mandatory)][string]$CandidateDirectory,
    [string]$GameRoot='C:\Program Files (x86)\Steam\steamapps\common\Singularity',
    [Parameter(Mandatory)][string]$AcceptedProfilePath,
    [Parameter(Mandatory)][string]$EvidenceDirectory
)
# MAIN-only guarded audio lease. Exact audio targets + one exact story lookup.
# Does not launch Steam, modify native banks, DLL, INI, text or game packages.
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'

function Hash([string]$Path){(Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()}
function Assert-Closed {
    if(@(Get-Process -Name Singularity -ErrorAction SilentlyContinue).Count){throw 'Game running; no audio writes'}
}
function Safe-Child([string]$Root,[string]$Relative){
    if($Relative -notmatch '^[A-Za-z0-9_./-]+$'){throw 'Unsafe relative path'}
    $base=[IO.Path]::GetFullPath($Root).TrimEnd('\')+'\'
    $full=[IO.Path]::GetFullPath((Join-Path $Root ($Relative -replace '/', '\')))
    if(-not $full.StartsWith($base,[StringComparison]::OrdinalIgnoreCase)){throw 'Path escapes owned root'}
    $parent=Split-Path $full
    while($parent.Length -ge $base.TrimEnd('\').Length){
        if(Test-Path -LiteralPath $parent){if((Get-Item -LiteralPath $parent).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Reparse point refused'}}
        $next=Split-Path $parent;if($next -eq $parent){break};$parent=$next
    }
    return $full
}
function Copy-Verified([string]$Source,[string]$Destination,[string]$Expected){
    Assert-Closed
    if((Hash $Source) -cne $Expected){throw "Source hash mismatch: $Source"}
    $temporary=$Destination+".xtts-$PID.new"
    if(Test-Path -LiteralPath $temporary){throw 'Temporary destination already exists'}
    try{
        Copy-Item -LiteralPath $Source -Destination $temporary
        if((Hash $temporary) -cne $Expected){throw 'Temporary copy hash mismatch'}
        Assert-Closed
        Move-Item -LiteralPath $temporary -Destination $Destination -Force
        if((Hash $Destination) -cne $Expected){throw 'Destination hash mismatch'}
    }finally{if(Test-Path -LiteralPath $temporary){Remove-Item -LiteralPath $temporary -Force}}
}

$game=(Resolve-Path -LiteralPath $GameRoot).Path
$candidate=(Resolve-Path -LiteralPath $CandidateDirectory).Path
$evidence=[IO.Path]::GetFullPath($EvidenceDirectory)
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$buildRoot=[IO.Path]::GetFullPath((Join-Path $repo 'build'))
if(-not $candidate.StartsWith($buildRoot+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Candidate must stay under this checkout/build'}
if(-not $evidence.StartsWith([IO.Path]::GetFullPath((Join-Path $repo 'deployment'))+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Evidence must stay under this checkout/deployment'}
$receipt=Join-Path $evidence 'xtts-audio-lease.json'
Assert-Closed

if($Action -eq 'Rollback'){
    $plan=Get-Content -LiteralPath $receipt -Raw | ConvertFrom-Json
    if($plan.Kind -cne 'SingularityVR XTTS audio candidate lease v1' -or $plan.GameRoot -ine $game -or
       @($plan.Files).Count -ne 1770 -or $plan.LookupPath -cne 'Binaries/vr-assets/localization/pl/polish_story_voice.jsonl'){
        throw 'Wrong XTTS rollback receipt'
    }
    $backupRoot=Join-Path $evidence 'backup'
    if((Hash (Join-Path $backupRoot 'polish_story_voice.jsonl')) -cne $plan.LookupBeforeSha256){throw 'Story lookup backup hash invalid'}
    foreach($f in $plan.Files){
        if($f.RelativePath -notmatch '^Binaries/vr-assets/audio/polish/story/[A-Za-z0-9_.-]+\.wav$' -or
           $f.CandidateSha256 -notmatch '^[0-9a-f]{64}$' -or
           ($f.BeforeSha256 -and $f.BeforeSha256 -notmatch '^[0-9a-f]{64}$')){throw 'Unsafe rollback file record'}
        if($f.BeforeSha256){
            $backup=Safe-Child $backupRoot ('audio/'+$f.Name)
            if((Hash $backup) -cne $f.BeforeSha256){throw 'Audio backup hash invalid'}
        }
    }
    $conflicts=[Collections.Generic.List[string]]::new()
    foreach($f in $plan.Files){
        $target=Safe-Child $game $f.RelativePath
        if(-not (Test-Path -LiteralPath $target)){
            if($f.BeforeSha256){$conflicts.Add($f.RelativePath)}
        }else{
            $actual=Hash $target
            if($actual -cne $f.CandidateSha256 -and $actual -cne $f.BeforeSha256){$conflicts.Add($f.RelativePath)}
        }
    }
    $lookup=Safe-Child $game $plan.LookupPath;$lookupHash=Hash $lookup
    if($lookupHash -cne $plan.LookupCandidateSha256 -and $lookupHash -cne $plan.LookupBeforeSha256){$conflicts.Add($plan.LookupPath)}
    if($conflicts.Count){throw ('Unexpected target changes retained; nothing restored. Conflicts='+$conflicts.Count)}
    if(-not $PSCmdlet.ShouldProcess($game,'Restore exact XTTS WAV and story lookup backups')){return}
    foreach($f in $plan.Files){
        Assert-Closed;$target=Safe-Child $game $f.RelativePath
        if($f.BeforeSha256){
            if((Hash $target) -cne $f.BeforeSha256){Copy-Verified (Safe-Child $backupRoot ('audio/'+$f.Name)) $target $f.BeforeSha256}
        }elseif(Test-Path -LiteralPath $target){Remove-Item -LiteralPath $target -Force}
    }
    if((Hash $lookup) -cne $plan.LookupBeforeSha256){Copy-Verified (Join-Path $backupRoot 'polish_story_voice.jsonl') $lookup $plan.LookupBeforeSha256}
    Write-Output 'RESTORED: XTTS audio targets and story lookup verified';return
}

$profilePath=(Resolve-Path -LiteralPath $AcceptedProfilePath).Path
$profile=Get-Content -LiteralPath $profilePath -Raw | ConvertFrom-Json
$manifestPath=Safe-Child $candidate 'audio-payload-manifest.json'
$manifestHash=Hash $manifestPath;$manifest=Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if($manifest.schema -cne 'singularityvr-xtts-audio-candidate-v1' -or $manifest.status -cne 'OFFLINE_STAGED_STEAM_AND_PHYSICAL_AUDIO_PENDING' -or
   @($manifest.files).Count -ne 1770 -or $manifest.lookupRowsUpdated -ne 103 -or
   (Hash $profilePath) -cne $manifest.acceptedProfileSha256){throw 'XTTS audio candidate/profile identity mismatch'}
$lookupPath='Binaries/vr-assets/localization/pl/polish_story_voice.jsonl'
$lookup=Safe-Child $game $lookupPath
$lookupPin=@($profile.files | Where-Object {$_.path -ceq $lookupPath})
if(-not $profile.approved -or $lookupPin.Count -ne 1 -or (Hash $lookup) -cne $manifest.lookupBeforeSha256 -or
   $lookupPin[0].sha256.ToLowerInvariant() -cne $manifest.lookupBeforeSha256){throw 'Installed lookup/accepted profile mismatch'}
$lookupCandidate=Safe-Child $candidate 'payload/Binaries/vr-assets/localization/pl/polish_story_voice.jsonl'
if((Hash $lookupCandidate) -cne $manifest.lookupAfterSha256){throw 'Staged lookup hash mismatch'}
$audioRoot=Safe-Child $game 'Binaries/vr-assets/audio/polish/story'
$files=@();$seen=@{}
foreach($f in $manifest.files){
    if($f.relativePath -notmatch '^Binaries/vr-assets/audio/polish/story/([A-Za-z0-9_.-]+\.wav)$' -or
       $Matches[1] -cne $f.name -or $f.candidateSha256 -notmatch '^[0-9a-f]{64}$' -or
       ($f.beforeSha256 -and $f.beforeSha256 -notmatch '^[0-9a-f]{64}$')){throw 'Invalid audio payload manifest entry'}
    if($seen.ContainsKey($f.relativePath.ToLowerInvariant())){throw 'Duplicate audio target'};$seen[$f.relativePath.ToLowerInvariant()]=$true
    $src=Safe-Child $candidate ('payload/'+$f.relativePath)
    if((Hash $src) -cne $f.candidateSha256){throw "Candidate audio hash mismatch: $($f.name)"}
    $target=Safe-Child $game $f.relativePath
    $before=if(Test-Path -LiteralPath $target){Hash $target}else{$null}
    if($before -cne $f.beforeSha256){throw "Installed audio baseline drift: $($f.name)"}
    $files+=@{Name=$f.name;RelativePath=$f.relativePath;BeforeSha256=$before;CandidateSha256=$f.candidateSha256}
}
if($files.Count -ne 1770){throw 'Audio inventory mismatch'}
Write-Output 'PLAN PASS: 1667 story + 97 skipped + 6 vignette WAVs; 103 exact lookup updates; accepted lookup and all source hashes verified'
if($Action -eq 'Plan' -or $WhatIfPreference){return}
if(Test-Path -LiteralPath $evidence){throw 'Refusing existing evidence directory'}
if(-not $PSCmdlet.ShouldProcess($game,'Install 1770 candidate audio files and exact story lookup with verified backups')){return}
New-Item -ItemType Directory -Path (Join-Path $evidence 'backup/audio') -Force | Out-Null
$lookupBackup=Join-Path $evidence 'backup/polish_story_voice.jsonl'
Copy-Item -LiteralPath $lookup -Destination $lookupBackup
if((Hash $lookupBackup) -cne $manifest.lookupBeforeSha256){throw 'Lookup backup hash mismatch'}
foreach($f in $files){
    if($f.BeforeSha256){$source=Safe-Child $game $f.RelativePath;$backup=Safe-Child (Join-Path $evidence 'backup') ('audio/'+$f.Name)
        Copy-Item -LiteralPath $source -Destination $backup
        if((Hash $backup) -cne $f.BeforeSha256){throw "Audio backup hash mismatch: $($f.Name)"}}
}
$plan=[ordered]@{Kind='SingularityVR XTTS audio candidate lease v1';GameRoot=$game;CreatedAt=(Get-Date).ToString('o');CandidateDirectory=$candidate;CandidateAudioManifestSha256=$manifestHash;LookupPath=$lookupPath;LookupBeforeSha256=$manifest.lookupBeforeSha256;LookupCandidateSha256=$manifest.lookupAfterSha256;Files=$files}
$plan | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $receipt -Encoding UTF8
Assert-Closed
try{
    # Audio files first; publish lookup last so it never points at uncopied files.
    foreach($f in $files){
        $target=Safe-Child $game $f.RelativePath
        $actual=if(Test-Path -LiteralPath $target){Hash $target}else{$null}
        if($actual -cne $f.BeforeSha256){throw "Installed audio changed during lease: $($f.Name)"}
        Copy-Verified (Safe-Child $candidate ('payload/'+$f.RelativePath)) $target $f.CandidateSha256
    }
    $lookupCandidate=Safe-Child $candidate 'payload/Binaries/vr-assets/localization/pl/polish_story_voice.jsonl'
    if((Hash $lookup) -cne $manifest.lookupBeforeSha256){throw 'Story lookup changed during lease'}
    Copy-Verified $lookupCandidate $lookup $manifest.lookupAfterSha256
    foreach($f in $files){if((Hash (Safe-Child $game $f.RelativePath)) -cne $f.CandidateSha256){throw 'Final WAV hash mismatch'}}
    if((Hash $lookup) -cne $manifest.lookupAfterSha256){throw 'Final story lookup hash mismatch'}
}catch{
    $failure=$_
    if(@(Get-Process -Name Singularity -ErrorAction SilentlyContinue).Count){throw "Install interrupted while game is running; keep lease for rollback after closing. $failure"}
    & $PSCommandPath -Action Rollback -CandidateDirectory $candidate -GameRoot $game -AcceptedProfilePath $profilePath -EvidenceDirectory $evidence
    throw $failure
}
Write-Output "INSTALLED: 1770 XTTS WAVs + 103 lookup rows; receipt $receipt"
