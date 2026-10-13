param([Parameter(Mandatory)][string]$OutputDirectory)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
if(Test-Path $OutputDirectory){throw 'Refuse existing fixture directory'}
$root=New-Item -ItemType Directory -Path $OutputDirectory
$candidate=New-Item -ItemType Directory -Path (Join-Path $root 'candidate')
$game=New-Item -ItemType Directory -Path (Join-Path $root 'game')
$lookup=New-Item -ItemType Directory -Path (Join-Path $game 'Binaries/vr-assets/localization/pl') -Force
$uiDir=New-Item -ItemType Directory -Path (Join-Path $game 'Binaries/vr-assets/audio/polish/ui') -Force
$storyDir=New-Item -ItemType Directory -Path (Join-Path $game 'Binaries/vr-assets/audio/polish/story') -Force
$wav=[byte[]](82,73,70,70,38,0,0,0,87,65,86,69,102,109,116,32,16,0,0,0,1,0,1,0,192,93,0,0,128,187,0,0,2,0,16,0,100,97,116,97,2,0,0,0,1,0)
$native=$wav.Clone();$native[44]=2
$rows=@();$qa=@();$ui=@();$story=@()
foreach($i in 0..1885){
    $id='line'+$i;$name='old'+$i+'.wav';$file=Join-Path $candidate ('new'+$i+'.wav')
    $isUi=$i -lt 894;$group=if($isUi){'full_localization'}else{'story_nllb'}
    [IO.File]::WriteAllBytes($file,$wav)
    $dir=if($isUi){$uiDir}else{$storyDir}
    [IO.File]::WriteAllBytes((Join-Path $dir $name),$native)
    $rows += @{pack=$group;line_id=$id;previous_audio=('old/'+$name);file=$file}|ConvertTo-Json -Compress
    $qa += @{line_id=$id;file=$file;qa_status='PASS_AUTOMATED_SCREEN'}
    if($isUi){$ui += @{id=$id;file=$name}|ConvertTo-Json -Compress}else{$story += @{source=$id;file=$name}|ConvertTo-Json -Compress}
}
$rows|Set-Content (Join-Path $candidate 'manifest.jsonl') -Encoding UTF8
$qa|ConvertTo-Json -Depth 3|Set-Content (Join-Path $candidate 'qa.json') -Encoding UTF8
$ui|Set-Content (Join-Path $lookup 'polish_voice.jsonl') -Encoding UTF8
$story|Set-Content (Join-Path $lookup 'polish_story_voice.jsonl') -Encoding UTF8
$installer=Join-Path (Split-Path $PSScriptRoot) 'polish/install-revoice-candidate.ps1'
& $installer -Action Install -CandidateDirectory $candidate -GameRoot $game -EvidenceDirectory (Join-Path $root 'whatif') -WhatIf
if(Test-Path (Join-Path $root 'whatif')){throw 'WhatIf wrote evidence'}
$lease=Join-Path $root 'lease'
& $installer -Action Install -CandidateDirectory $candidate -GameRoot $game -EvidenceDirectory $lease
$plan=Get-Content (Join-Path $lease 'revoice-lease.json') -Raw|ConvertFrom-Json
if($plan.PendingQa -ne 0){throw 'QA count regression'}
foreach($f in $plan.Files){if((Get-FileHash (Join-Path $game $f.RelativePath)).Hash -cne $f.CandidateSha256){throw 'Install verification'}}
$one=$plan.Files[0];$b=Join-Path $lease $one.Backup
[IO.File]::WriteAllBytes($b,$wav)
$failed=$false
try{& $installer -Action Rollback -GameRoot $game -EvidenceDirectory $lease}catch{$failed=$true}
if(-not $failed){throw 'Corrupt backup accepted'}
foreach($f in $plan.Files){if((Get-FileHash (Join-Path $game $f.RelativePath)).Hash -cne $f.CandidateSha256){throw 'Partial restore before backup validation'}}
[IO.File]::WriteAllBytes($b,$native)
$conflict=$wav.Clone();$conflict[44]=3
[IO.File]::WriteAllBytes((Join-Path $game $one.RelativePath),$conflict)
$failed=$false
try{& $installer -Action Rollback -GameRoot $game -EvidenceDirectory $lease}catch{$failed=$true}
if(-not $failed -or [IO.File]::ReadAllBytes((Join-Path $game $one.RelativePath))[44] -ne 3){throw 'Unexpected user WAV overwritten'}
foreach($f in $plan.Files|Select-Object -Skip 1){if((Get-FileHash (Join-Path $game $f.RelativePath)).Hash -cne $f.BaselineSha256){throw 'Owned originals not restored'}}
[IO.File]::WriteAllBytes((Join-Path $game $one.RelativePath),$native)
& $installer -Action Rollback -GameRoot $game -EvidenceDirectory $lease
Write-Output 'PASS: 1886 WAV-only install/restore, WhatIf, corrupt backup prevalidation, preserved user conflict, QA count'
