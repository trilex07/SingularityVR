Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$repo=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
. (Join-Path $repo 'tools/deployment/AcceptedProfileGuard.ps1')
$testRoot=Join-Path $repo ('build/accepted-profile-guard/'+[guid]::NewGuid().ToString('N'))
[void][IO.Directory]::CreateDirectory($testRoot)
$game=Join-Path $testRoot 'fake-steam/steamapps/common/Singularity'
$bin=Join-Path $game 'Binaries';[void][IO.Directory]::CreateDirectory($bin)
$utf8=[Text.UTF8Encoding]::new($false)
function Write-Fixture([string]$Path,[string]$Text){
    [void][IO.Directory]::CreateDirectory((Split-Path -Parent $Path));[IO.File]::WriteAllText($Path,$Text,$utf8)
}
function Write-JsonFixture([string]$Path,$Object){Write-Fixture $Path ($Object | ConvertTo-Json -Depth 12)}
function Assert-True([bool]$Value,[string]$Why){if(-not $Value){throw $Why}}
$script:checks=0
function Must-Reject([scriptblock]$Action,[string]$Why){
    $rejected=$false;try{& $Action | Out-Null}catch{$rejected=$true}
    if(-not $rejected){throw "Did not reject: $Why"};$script:checks++
}
$iniText=@'
[Render]
GloveNativeEnabled=1
GloveRendererEnabled=1
KnifeHandGripEnabled=1
HandThumbPoseFix=1
KnifeSceneLighting=1
AutoResX=5000
VrCameraShakeSuppression=0
HealthPlateEnabled=1
AmmoPlateEnabled=1
[Input]
HipKnifeEnabled=1
GunAlign0Fwd=26
[Localization]
PolishLanguage=1
NativePolishSpeech=1
[Diagnostic]
IndependentHandsAudit=0
'@
$iniPath=Join-Path $bin 'SingularityVR.ini';Write-Fixture $iniPath $iniText
$dllPath=Join-Path $bin 'd3d9.dll';Write-Fixture $dllPath 'synthetic accepted DLL, not executable'
$paths=@('Binaries/d3d9.dll',
 'Binaries/vr-assets/hud/katorga/glove-animation/native_hands.svni',
 'Binaries/vr-assets/hud/katorga/glove-animation/hand_thumb.svnt',
 'Binaries/vr-assets/knife/knife.svrk','Binaries/vr-assets/knife/knife.svrt',
 'Binaries/vr-assets/knife/knife_hand.svng','Binaries/vr-assets/knife/knife.svnl',
 'Binaries/vr-assets/knife/knife_normal.svrt','Binaries/vr-assets/knife/knife_specular.svrt',
 'Binaries/vr-assets/localization/pl/polish_text.jsonl',
 'Binaries/vr-assets/localization/pl/polish_voice.jsonl',
 'Binaries/vr-assets/localization/pl/polish_story_voice.jsonl','RvGame/CookedPC/ui_GFx_fonts.xxx')
$files=@()
foreach($relative in $paths){
    $path=Resolve-GuardPath $game $relative
    if($relative -ne 'Binaries/d3d9.dll'){Write-Fixture $path ('synthetic '+$relative)}
    $files += [pscustomobject]@{path=$relative;sha256=(Get-GuardHash $path)}
}
$profile=[pscustomobject]@{schema='singularityvr-accepted-profile-v1';approved=$true;
 acceptedBy='synthetic test';acceptanceScope='CPU fixture only';iniPath='Binaries/SingularityVR.ini';
 ini=(Read-GuardIni $iniPath);ignoredIniKeys=@('Render/AutoResX');files=$files}
$profilePath=Join-Path $testRoot 'accepted.json';Write-JsonFixture $profilePath $profile
$baselineIniHash=Get-GuardHash $iniPath;$baselineDllHash=Get-GuardHash $dllPath
$candidateDll=Join-Path $testRoot 'candidate/d3d9.dll';Write-Fixture $candidateDll 'synthetic candidate DLL'
$candidateIni=Join-Path $testRoot 'candidate/SingularityVR.ini'
Write-Fixture $candidateIni ($iniText.Replace('VrCameraShakeSuppression=0','VrCameraShakeSuppression=1'))
$plan=[pscustomobject]@{schema='singularityvr-change-plan-v1';profileSha256=(Get-GuardHash $profilePath);
 reason='synthetic camera test only';files=@([pscustomobject]@{path='Binaries/d3d9.dll';before=$baselineDllHash;
 after=(Get-GuardHash $candidateDll);reason='tested candidate'});
 ini=@([pscustomobject]@{key='Render/VrCameraShakeSuppression';before='0';after='1';reason='single experiment'})}
$planPath=Join-Path $testRoot 'plan.json';Write-JsonFixture $planPath $plan
$map=@([pscustomobject]@{Target='Binaries/d3d9.dll';Source=$candidateDll},
 [pscustomobject]@{Target='Binaries/SingularityVR.ini';Source=$candidateIni})
function Preflight {
    Assert-AcceptedDeployment -GameRoot $game -ProfilePath $profilePath -ChangePlanPath $planPath -FileMap $map
}
$ticket=Preflight
Assert-True ($ticket.Sources.Count -eq 2) 'candidate map missing'
$draft=New-AcceptedProfileDraft -GameRoot $game -RelativeFiles $paths
Assert-True (-not $draft.approved -and -not $draft.acceptedBy) 'draft silently promoted acceptance'
$draftPath=Join-Path $testRoot 'draft.json';Write-JsonFixture $draftPath $draft
Must-Reject {Assert-AcceptedDeployment -GameRoot $game -ProfilePath $draftPath} 'generated draft is not accepted'
foreach($key in @('GloveNativeEnabled','GloveRendererEnabled','KnifeHandGripEnabled',
 'HandThumbPoseFix','KnifeSceneLighting','HipKnifeEnabled','PolishLanguage','NativePolishSpeech',
 'HealthPlateEnabled','AmmoPlateEnabled')){
    Write-Fixture $candidateIni (($iniText.Replace('VrCameraShakeSuppression=0','VrCameraShakeSuppression=1')).Replace("$key=1","$key=0"))
    Must-Reject {Preflight} "regressed feature $key"
}
Write-Fixture $candidateIni ($iniText.Replace('VrCameraShakeSuppression=0','VrCameraShakeSuppression=1').Replace('GunAlign0Fwd=26',''))
Must-Reject {Preflight} 'lost calibration'
Write-Fixture $candidateIni ($iniText.Replace('VrCameraShakeSuppression=0','VrCameraShakeSuppression=1').Replace('IndependentHandsAudit=0','IndependentHandsAudit=1'))
Must-Reject {Preflight} 'unexpected diagnostics'
Write-Fixture $candidateIni ($iniText.Replace('VrCameraShakeSuppression=0','VrCameraShakeSuppression=1'))
Assert-True ((Get-GuardHash $iniPath) -eq $baselineIniHash -and (Get-GuardHash $dllPath) -eq $baselineDllHash) 'preflight wrote live files'
$asset=Resolve-GuardPath $game $paths[1];$assetText=[IO.File]::ReadAllText($asset)
Write-Fixture $asset 'tampered asset';Must-Reject {Preflight} 'asset drift';[IO.File]::Delete($asset)
Must-Reject {Preflight} 'missing asset';Write-Fixture $asset $assetText
Write-Fixture $iniPath ($iniText.Replace('PolishLanguage=1','PolishLanguage=0'))
Must-Reject {Preflight} 'live installation already regressed';Write-Fixture $iniPath $iniText
Write-Fixture $iniPath ($iniText.Replace('AutoResX=5000','AutoResX=5264'));[void](Preflight)
Write-Fixture $iniPath $iniText
Write-Fixture $candidateIni ($iniText+"`n[Render]`nGloveNativeEnabled=0")
Must-Reject {Preflight} 'duplicate INI key';Write-Fixture $candidateIni ($iniText.Replace('VrCameraShakeSuppression=0','VrCameraShakeSuppression=1'))
$plan.profileSha256='0'*64;Write-JsonFixture $planPath $plan;Must-Reject {Preflight} 'stale profile pin'
$plan.profileSha256=Get-GuardHash $profilePath;Write-JsonFixture $planPath $plan
$savedReason=$plan.files[0].reason;$plan.files[0].reason='';Write-JsonFixture $planPath $plan
Must-Reject {Preflight} 'missing change reason';$plan.files[0].reason=$savedReason;Write-JsonFixture $planPath $plan
$map += [pscustomobject]@{Target='../escape.dll';Source=$candidateDll}
Must-Reject {Preflight} 'path traversal';$map=$map[0..1]
$map += [pscustomobject]@{Target='Binaries/unlisted.dll';Source=$candidateDll}
Must-Reject {Preflight} 'unlisted write';$map=$map[0..1]
$savedTarget=$plan.files[0].path;$plan.files[0].path='Binaries/*.dll';Write-JsonFixture $planPath $plan
Must-Reject {Preflight} 'wildcard allowance';$plan.files[0].path=$savedTarget;Write-JsonFixture $planPath $plan
$ticket=Preflight
Write-Fixture $candidateDll 'changed source';Must-Reject {Assert-GuardWriteTicket $ticket $dllPath $candidateDll} 'source race'
Write-Fixture $candidateDll 'synthetic candidate DLL'
Write-Fixture $dllPath 'changed target';Must-Reject {Assert-GuardWriteTicket $ticket $dllPath $candidateDll} 'target race'
Write-Fixture $dllPath 'synthetic accepted DLL, not executable'
$plan.reason='changed plan';Write-JsonFixture $planPath $plan
Must-Reject {Assert-GuardWriteTicket $ticket $dllPath $candidateDll} 'plan race'
$plan.reason='synthetic camera test only';Write-JsonFixture $planPath $plan
$oldApproved=$profile.approved;$profile.approved=$false;Write-JsonFixture $profilePath $profile
Must-Reject {Preflight} 'unapproved draft';$profile.approved=$oldApproved;Write-JsonFixture $profilePath $profile
$plan.profileSha256=Get-GuardHash $profilePath;Write-JsonFixture $planPath $plan
$profile.ignoredIniKeys=@('Render/*');Write-JsonFixture $profilePath $profile
$plan.profileSha256=Get-GuardHash $profilePath;Write-JsonFixture $planPath $plan
Must-Reject {Preflight} 'broad ignore';$profile.ignoredIniKeys=@('Render/AutoResX');Write-JsonFixture $profilePath $profile
$plan.profileSha256=Get-GuardHash $profilePath;Write-JsonFixture $planPath $plan
# Synthetic install/restore: no runner, Steam, process inspection or real assets.
$ticket=Preflight
Assert-GuardWriteTicket $ticket $dllPath $candidateDll
Assert-GuardWriteTicket $ticket $iniPath $candidateIni
[IO.File]::Copy($candidateDll,$dllPath,$true);[IO.File]::Copy($candidateIni,$iniPath,$true)
[void](Assert-AcceptedDeployment -GameRoot $game -ProfilePath $profilePath -ChangePlanPath $planPath -FileMap $map -Phase Post)
$restoreDll=Join-Path $testRoot 'restore/d3d9.dll';Write-Fixture $restoreDll 'synthetic accepted DLL, not executable'
$restoreIni=Join-Path $testRoot 'restore/SingularityVR.ini';Write-Fixture $restoreIni $iniText
$reverse=@([pscustomobject]@{Target='Binaries/d3d9.dll';Source=$restoreDll},[pscustomobject]@{Target='Binaries/SingularityVR.ini';Source=$restoreIni})
$restore=Assert-AcceptedDeployment -GameRoot $game -ProfilePath $profilePath -ChangePlanPath $planPath -FileMap $reverse -Direction Rollback
Write-Fixture $dllPath 'newer promoted build';Must-Reject {
    Assert-AcceptedDeployment -GameRoot $game -ProfilePath $profilePath -ChangePlanPath $planPath -FileMap $reverse -Direction Rollback
} 'stale rollback over promoted build'
Write-Fixture $dllPath 'synthetic candidate DLL'
Assert-GuardWriteTicket $restore $dllPath $restoreDll;Assert-GuardWriteTicket $restore $iniPath $restoreIni
[IO.File]::Copy($restoreDll,$dllPath,$true);[IO.File]::Copy($restoreIni,$iniPath,$true)
[void](Assert-AcceptedDeployment -GameRoot $game -ProfilePath $profilePath -ChangePlanPath $planPath -FileMap $reverse -Direction Rollback -Phase Post)
Assert-True ((Get-GuardHash $iniPath) -eq $baselineIniHash -and (Get-GuardHash $dllPath) -eq $baselineDllHash) 'restore not exact'
foreach($relative in @('../bad','Binaries/../bad','C:/bad','Binaries/a:stream','Binaries/file.','Binaries/CON','/root')){
    Must-Reject {Resolve-GuardPath $game $relative} "unsafe path $relative"
}
foreach($path in @('tools/deploy-singularityvr.ps1','tools/deployment/AcceptedProfileGuard.ps1')){
    $tokens=$null;$errors=$null
    [void][Management.Automation.Language.Parser]::ParseFile((Join-Path $repo $path),[ref]$tokens,[ref]$errors)
    Assert-True ($errors.Count -eq 0) "PowerShell parse failed: $path"
}
# Exercise the actual runner's read-only path against the FAKE Steam tree.
# Minimal synthetic PE header passes the runner's architecture gate, not a DLL
# that can execute. No Get-Process/CIM or launch is reached in WhatIf mode.
$fakePe=New-Object byte[] 512;$fakePe[0]=0x4d;$fakePe[1]=0x5a
[BitConverter]::GetBytes([int]128).CopyTo($fakePe,0x3c)
[BitConverter]::GetBytes([uint32]0x00004550).CopyTo($fakePe,128)
[BitConverter]::GetBytes([uint16]0x014c).CopyTo($fakePe,132)
[BitConverter]::GetBytes([uint16]0x010b).CopyTo($fakePe,152)
[IO.File]::WriteAllBytes($candidateDll,$fakePe)
$plan.files[0].after=Get-GuardHash $candidateDll;$plan.ini=@();Write-JsonFixture $planPath $plan
$runner=Join-Path $repo 'tools/deploy-singularityvr.ps1'
$countBefore=@(Get-ChildItem -LiteralPath $testRoot -File -Recurse).Count
& $runner -DllOnly -SteamRoot (Join-Path $testRoot 'fake-steam') -ArtifactDir (Split-Path -Parent $candidateDll) `
    -AcceptedProfilePath $profilePath -ChangePlanPath $planPath -WhatIf
Assert-True (@(Get-ChildItem -LiteralPath $testRoot -File -Recurse).Count -eq $countBefore) 'WhatIf created files'
Assert-True ((Get-GuardHash $dllPath) -eq $baselineDllHash) 'WhatIf wrote DLL'
Write-Fixture $iniPath ($iniText.Replace('PolishLanguage=1','PolishLanguage=0'))
Must-Reject {
    & $runner -DllOnly -SteamRoot (Join-Path $testRoot 'fake-steam') -ArtifactDir (Split-Path -Parent $candidateDll) `
        -AcceptedProfilePath $profilePath -ChangePlanPath $planPath -WhatIf
} 'actual runner rejected regressed live INI before process/writes'
Write-Fixture $iniPath $iniText
Must-Reject { & (Join-Path $repo 'spikes/view_matrix/build.ps1') -Install } 'legacy build Install bypass disabled before SDK/build/writes'
$cuffProfile=Read-GuardJson $profilePath
$cuffProfile.ini | Add-Member -NotePropertyName 'Render/NativeGloveCuffClosure' -NotePropertyValue '1'
$cuffProfilePath=Join-Path $testRoot 'cuff-profile-without-asset.json';Write-JsonFixture $cuffProfilePath $cuffProfile
$dependencyError=''
try{[void](Assert-AcceptedDeployment -GameRoot $game -ProfilePath $cuffProfilePath)}catch{$dependencyError=$_.Exception.Message}
Assert-True ($dependencyError -like 'Enabled feature dependency is not pinned:*native_cuffs.svnc') 'cuff feature did not require its exact dependent asset'
$script:checks++
Write-Output "PASS: $script:checks fail-closed cases; semantic INI preservation, pinned assets, pre/post, exact synthetic rollback; ZERO Steam. Fixtures: $testRoot"
