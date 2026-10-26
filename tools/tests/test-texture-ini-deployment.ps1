# Executes the production deploy/rollback against a private fake Steam tree.
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
. "$repo/tools/deployment/AcceptedProfileGuard.ps1"
$fixture=Join-Path $repo ('build/texture-ini-lease/'+[guid]::NewGuid().ToString('N'))
$steam="$fixture/steam"; $game="$steam/steamapps/common/Singularity"; $bin="$game/Binaries"; $stage="$fixture/stage"
[void][IO.Directory]::CreateDirectory($bin); [void][IO.Directory]::CreateDirectory($stage)
function WriteJson($path,$value){[IO.File]::WriteAllText($path,($value|ConvertTo-Json -Depth 12))}
function Hash($path){(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash}
function Check($yes,$why){if(-not $yes){throw $why}}
# Non-executable PE-header fixture. It is never loaded or launched.
$pe=[byte[]]::new(512); $pe[0]=77;$pe[1]=90;$pe[60]=128
$pe[128]=80;$pe[129]=69;$pe[132]=76;$pe[133]=1;$pe[152]=11;$pe[153]=1
foreach($name in @('d3d9.dll','dinput8.dll')){
 [IO.File]::WriteAllBytes("$bin/$name",$pe)
 $candidate=[byte[]]$pe.Clone();$candidate[511]=1
 [IO.File]::WriteAllBytes("$stage/$name",$candidate)
}
$ini="[Render]`nGloveNativeEnabled=0`nGloveRendererEnabled=0`nKnifeHandGripEnabled=0`nHandThumbPoseFix=0`nKnifeSceneLighting=0`n[Input]`nHipKnifeEnabled=0`n[Localization]`nPolishLanguage=0`n"
[IO.File]::WriteAllText("$bin/SingularityVR.ini",$ini)
[IO.File]::WriteAllText("$bin/texture_override.ini","[general]`nvalkyrie_material_mode=0`n")
[IO.File]::WriteAllText("$stage/texture_override.ini","[general]`nvalkyrie_material_mode=2`n")
$names=@('d3d9.dll','dinput8.dll','texture_override.ini')
$files=@($names|ForEach-Object{[pscustomobject]@{path="Binaries/$_";sha256=(Hash "$bin/$_")}})
$profile=@{schema='singularityvr-accepted-profile-v1';approved=$true;acceptedBy='synthetic fixture';acceptanceScope='offline fake Steam only';iniPath='Binaries/SingularityVR.ini';ini=(Read-GuardIni "$bin/SingularityVR.ini");ignoredIniKeys=@();files=$files}
WriteJson "$fixture/profile.json" $profile
$changes=@($names|ForEach-Object{[pscustomobject]@{path="Binaries/$_";before=(Hash "$bin/$_");after=(Hash "$stage/$_");reason='fixture'}})
WriteJson "$fixture/plan.json" @{schema='singularityvr-change-plan-v1';profileSha256=(Hash "$fixture/profile.json");reason='offline fixture';files=$changes;ini=@()}
$common=@('-NoProfile','-File',"$repo/tools/deploy-singularityvr.ps1",'-SteamRoot',$steam,'-RepoRoot',$repo,'-ArtifactDir',$stage,'-AcceptedProfilePath',"$fixture/profile.json",'-ChangePlanPath',"$fixture/plan.json")
$iniHash=Hash "$bin/SingularityVR.ini"
$log=& pwsh @common -RuntimeDllPairOnly -IncludeTextureIni -WhatIf 2>&1
Check ($LASTEXITCODE -eq 0) "Preflight failed: $log"
foreach($f in $files){Check ((Hash "$game/$($f.path)") -eq $f.sha256) 'WhatIf wrote a file'}
$log=& pwsh @common -RuntimeDllPairOnly -IncludeTextureIni 2>&1
Check ($LASTEXITCODE -eq 0) "Install failed: $log"
$backupLine=@($log|Where-Object{$_ -like 'Backup: *'})[-1].ToString()
$backup=$backupLine.Substring(8)
foreach($f in $changes){Check ((Hash "$game/$($f.path)") -eq $f.after) 'Candidate hash mismatch'}
Check ((Hash "$bin/SingularityVR.ini") -eq $iniHash) 'Unrelated INI changed'
$log=& pwsh @common -Action Rollback -BackupPath $backup 2>&1
Check ($LASTEXITCODE -eq 0) "Rollback failed: $log"
foreach($f in $files){Check ((Hash "$game/$($f.path)") -eq $f.sha256) 'Restore hash mismatch'}
[IO.File]::WriteAllText("$stage/texture_override.ini",'unreviewed drift')
$log=& pwsh @common -RuntimeDllPairOnly -IncludeTextureIni -WhatIf 2>&1
Check ($LASTEXITCODE -ne 0) 'Unreviewed config drift accepted'
foreach($f in $files){Check ((Hash "$game/$($f.path)") -eq $f.sha256) 'Rejected plan wrote files'}
Write-Host 'PASS: real deploy preflight/install/rollback, exact texture config, unrelated INI preserved, drift rejected. Fake Steam only.'
