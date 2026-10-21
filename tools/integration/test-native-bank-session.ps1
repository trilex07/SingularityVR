$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$id=[guid]::NewGuid().ToString('N')
$root=Join-Path $repo ('build/next-weapon-reload/native-guard-fixture-'+$id)
$runtime=Join-Path $repo ('deployment/install-backups/native-guard-fixture-'+$id)
$steam=Join-Path $root 'steam'
$game=Join-Path $steam 'steamapps/common/Singularity'
$candidate=Join-Path $root 'candidate'
$assets=Join-Path $root 'bank-backup'
function Put($p,$s){New-Item -ItemType Directory -Path (Split-Path -Parent $p) -Force | Out-Null;[IO.File]::WriteAllText($p,$s)}
function Hash($p){return (Get-FileHash -LiteralPath $p).Hash}
function Json($p,$v){Put $p ($v | ConvertTo-Json -Depth 14)}
function Get-CimInstance {param($ClassName,$Filter,$ErrorAction);if($Filter -like 'ProcessId=*'){return [pscustomobject]@{ProcessId=$PID}};return @()}
function Refuse([scriptblock]$call){$failed=$false;try{& $call | Out-Null}catch{$failed=$true};if(-not $failed){throw 'Expected refusal'}}
$profile=Get-Content -Raw (Join-Path $repo 'deployment/accepted-profile.json') | ConvertFrom-Json
foreach($f in $profile.files){
    $p=Join-Path $game $f.path
    if($null -ne $f.sha256){Put $p ('FAKE NON-EXECUTABLE '+$f.path);$f.sha256=Hash $p}
}
Put (Join-Path $game 'Binaries/Singularity.exe') 'FAKE NON-EXECUTABLE GAME; NEVER LAUNCHED'
New-Item -ItemType Directory -Path $runtime,$candidate -Force | Out-Null
Copy-Item (Join-Path $game 'Binaries/d3d9.dll') (Join-Path $runtime 'd3d9.dll')
$baseIni=Join-Path $repo 'deployment/install-backups/20261021-104518/SingularityVR.ini'
Copy-Item $baseIni (Join-Path $runtime 'SingularityVR.ini')
Copy-Item (Join-Path $repo 'build/combined-followups/kathryn-fix-5832409/SingularityVR.ini') (Join-Path $candidate 'SingularityVR.ini')
$profilePath=Join-Path $root 'profile.json';Json $profilePath $profile
$plan=Get-Content -Raw (Join-Path $repo 'build/combined-followups/kathryn-fix-5832409/change-plan.json') | ConvertFrom-Json
$plan.profileSha256=Hash $profilePath
$plan.files[0].before=Hash (Join-Path $runtime 'd3d9.dll')
Put (Join-Path $candidate 'd3d9.dll') 'FAKE CANDIDATE RENDERER; NEVER EXECUTED'
$plan.files[0].after=Hash (Join-Path $candidate 'd3d9.dll')
$corePlan=Join-Path $root 'core.json';Json $corePlan $plan
$banks=@($profile.files | Where-Object path -imatch '^RvGame/CookedPC/SP_.*_LOC_INT\.xxx$' | Select-Object -First 3)
if($banks.Count -ne 3){throw 'Missing native bank fixture pins'}
$rows=@();foreach($b in $banks){
    $source=Join-Path (Join-Path $candidate 'payload') $b.path
    Put $source ('FAKE CANDIDATE BANK '+$b.path)
    $rows += [pscustomobject]@{path=$b.path;before=$b.sha256.ToLowerInvariant();after=(Hash $source).ToLowerInvariant()}
}
$changed=@($rows.path)
$protected=@($profile.files | Where-Object {$_.path -cnotin $changed -and $_.path -cne 'Binaries/d3d9.dll' -and $null -ne $_.sha256} | ForEach-Object {@{path=$_.path;sha256=$_.sha256.ToLowerInvariant()}})
$manifest=Join-Path $candidate 'asset-manifest.json'
Json $manifest @{schema='singularityvr-session-assets-delta-v1';accepted_profile_sha256=(Hash $profilePath);files=$rows;protected=$protected}
$adapter=Join-Path $PSScriptRoot 'session-assets.ps1'
& $adapter -Action Deploy -ManifestPath $manifest -BackupDir $assets -GameRoot $game
Copy-Item (Join-Path $candidate 'd3d9.dll') (Join-Path $game 'Binaries/d3d9.dll') -Force
Copy-Item (Join-Path $candidate 'SingularityVR.ini') (Join-Path $game 'Binaries/SingularityVR.ini') -Force
$plan.files=@($plan.files)+@($rows | ForEach-Object {@{path=$_.path;before=$_.before;after=$_.after;reason='Exact fake native bank'}})
$combined=Join-Path $root 'combined.json';Json $combined $plan
$argsMap=@{CandidateDir=$candidate;RuntimeBackup=$runtime;AssetManifest=$manifest;AssetBackup=$assets;ProfilePath=$profilePath;CorePlan=$corePlan;CombinedPlan=$combined;BaselineIniSha256=(Hash $baseIni);SteamRoot=$steam;ValidateOnly=$true}
$guard=Join-Path $PSScriptRoot 'watch-native-bank-session.ps1'
& $guard @argsMap
Refuse {& $guard @argsMap -BaselineIniSha256 ('0'*64)}
$saved=Join-Path $assets $rows[0].path
$savedBytes=[IO.File]::ReadAllBytes($saved);Put $saved 'FOREIGN BACKUP'
Refuse {& $guard @argsMap}
[IO.File]::WriteAllBytes($saved,$savedBytes)
$live=Join-Path $game $rows[0].path
$liveBytes=[IO.File]::ReadAllBytes($live);Put $live 'FOREIGN LIVE BANK'
Refuse {& $guard @argsMap}
[IO.File]::WriteAllBytes($live,$liveBytes)
& $guard @argsMap
Write-Output 'PASS: fake-install combined lease preflight, wrong baseline, damaged backup, foreign live bank; no real Steam access'
