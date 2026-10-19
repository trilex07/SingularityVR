$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$root=Join-Path $repo ('build/followup-prep-tests-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $root | Out-Null
$script=Join-Path $repo 'tools/reload/prepare_combined_followups.ps1'
$checks=0
function Check($ok,$message){if(-not $ok){throw $message};$script:checks++}
function MakeCase([string]$name,[string]$extra=''){
 $dir=Join-Path $root $name;New-Item -ItemType Directory -Path $dir | Out-Null
 $ini=Join-Path $dir 'accepted.ini'
 [IO.File]::WriteAllText($ini,"[Input]`r`nKeepAcquiredWeapons=0`r`nGunAlign0PitchTenths=-28`r`n$extra[Render]`r`nNativeMuzzleParticleStereo=1`r`nDesktopMirrorWindow=1`r`n[Localization]`r`nPolishLanguage=1`r`n",[Text.UTF8Encoding]::new($false))
 $dll=Join-Path $dir 'd3d9.dll';[IO.File]::WriteAllBytes($dll,[byte[]](1,2,3))
 $sha=(Get-FileHash $dll -Algorithm SHA256).Hash
 $manifest=[ordered]@{sha256=$sha;architecture='x86 PE32';optimization='/O2';workingTreeStatus=@()}
 $manifest | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $dir 'd3d9.build.json') -Encoding UTF8
 $profile=[ordered]@{
  schema='singularityvr-accepted-profile-v1';approved=$true
  files=@(@{path='Binaries/d3d9.dll';sha256=$sha},@{path='Binaries/SingularityVR.ini';sha256=(Get-FileHash $ini -Algorithm SHA256).Hash})
  ini=@{'Input/KeepAcquiredWeapons'='0'}
 }
 $profilePath=Join-Path $dir 'profile.json';$profile | ConvertTo-Json -Depth 8 | Set-Content $profilePath -Encoding UTF8
 return @{InstalledIni=$ini;AcceptedProfilePath=$profilePath;CandidateDir=$dir}
}
$good=MakeCase 'good'; & $script @good | Out-Null
$text=[IO.File]::ReadAllText((Join-Path $good.CandidateDir 'SingularityVR.ini'))
foreach($line in @('KeepAcquiredWeapons=1','WeaponWheelOwnedInventory=1','NpcNameScalePct=60','CutsceneNativeHands=1','NativeMuzzleParticleStereo=1','DesktopMirrorWindow=1','PolishLanguage=1','GunAlign0PitchTenths=-28')){
 Check $text.Contains($line) "Missing or regressed $line"
}
$plan=Get-Content -Raw (Join-Path $good.CandidateDir 'change-plan.json') | ConvertFrom-Json
Check ($plan.files.Count -eq 1 -and $plan.files[0].path -ceq 'Binaries/d3d9.dll') 'Scope expanded beyond DLL/INI'
Check ($plan.ini.Count -eq 5) 'Unexpected key delta'
$failed=$false;try{& $script @good | Out-Null}catch{$failed=$true};Check $failed 'Overwrite accepted'
$duplicate=MakeCase 'duplicate' "KeepAcquiredWeapons=0`r`n"
$failed=$false;try{& $script @duplicate | Out-Null}catch{$failed=$true};Check $failed 'Duplicate key accepted'
Check (-not (Test-Path (Join-Path $duplicate.CandidateDir 'SingularityVR.ini'))) 'Partial output on duplicate'
$drift=MakeCase 'drift';[IO.File]::AppendAllText($drift.InstalledIni,'; changed')
$failed=$false;try{& $script @drift | Out-Null}catch{$failed=$true};Check $failed 'Baseline drift adopted'
$dirty=MakeCase 'dirty'
$m=Get-Content -Raw (Join-Path $dirty.CandidateDir 'd3d9.build.json') | ConvertFrom-Json
$m.workingTreeStatus=@(' M source.cpp');$m | ConvertTo-Json | Set-Content (Join-Path $dirty.CandidateDir 'd3d9.build.json') -Encoding UTF8
$failed=$false;try{& $script @dirty | Out-Null}catch{$failed=$true};Check $failed 'Dirty build accepted'
Write-Output "PASS $checks offline preparation checks; fake files only; no Steam access"
