[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$CandidateDir,
    [Parameter(Mandatory)][string]$RuntimeBackup,
    [Parameter(Mandatory)][string]$AssetManifest,
    [Parameter(Mandatory)][string]$AssetBackup,
    [Parameter(Mandatory)][string]$ProfilePath,
    [Parameter(Mandatory)][string]$CorePlan,
    [Parameter(Mandatory)][string]$CombinedPlan,
    [Parameter(Mandatory)][ValidatePattern('^[A-Fa-f0-9]{64}$')][string]$BaselineIniSha256,
    [string]$SteamRoot='C:\Program Files (x86)\Steam',
    [ValidateRange(60,1800)][int]$LaunchWindowSeconds=600,
    [switch]$ValidateOnly
)
# MAIN-only exact native-bank + DLL/INI lease. No launch/kill or broad restore.
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
. (Join-Path $repo 'tools/deployment/AcceptedProfileGuard.ps1')
$game=Join-Path $SteamRoot 'steamapps/common/Singularity'
$candidate=[IO.Path]::GetFullPath($CandidateDir)
$runtime=[IO.Path]::GetFullPath($RuntimeBackup)
$assets=[IO.Path]::GetFullPath($AssetBackup)
foreach($p in @($candidate,$AssetManifest,$assets,$CorePlan,$CombinedPlan)){
    if(-not ([IO.Path]::GetFullPath($p)).StartsWith($repo+'\build\',[StringComparison]::OrdinalIgnoreCase)){
        throw 'Private candidate/plan/asset paths must stay in this checkout build root'
    }
}
if(-not $runtime.StartsWith($repo+'\deployment\install-backups\',[StringComparison]::OrdinalIgnoreCase)){
    throw 'Runtime backup must belong to this checkout'
}
function Running {
    if(-not (Get-CimInstance Win32_Process -Filter "ProcessId=$PID" -ErrorAction Stop)){throw 'Full process visibility required'}
    return @((Get-CimInstance Win32_Process -Filter "Name='Singularity.exe'" -ErrorAction Stop)).Count -gt 0
}
[void](Running)
$m=Read-GuardJson $AssetManifest
$profile=Read-GuardJson $ProfilePath
$plan=Read-GuardJson $CorePlan
$combined=Read-GuardJson $CombinedPlan
if($m.schema -cne 'singularityvr-session-assets-delta-v1' -or
   $m.accepted_profile_sha256 -ine (Get-GuardHash $ProfilePath) -or
   $combined.profileSha256 -ine $plan.profileSha256 -or
   ($combined.ini | ConvertTo-Json -Depth 8 -Compress) -cne ($plan.ini | ConvertTo-Json -Depth 8 -Compress)){
    throw 'Unbound native-bank/core/combined plans'
}
$map=@([pscustomobject]@{Target='Binaries/d3d9.dll';Source=(Join-Path $candidate 'd3d9.dll')},
       [pscustomobject]@{Target='Binaries/SingularityVR.ini';Source=(Join-Path $candidate 'SingularityVR.ini')})
$expected=@($plan.files)
foreach($f in $m.files){
    if($f.path -inotmatch '^RvGame/CookedPC/SP_[A-Za-z0-9_]+_LOC_INT\.xxx$' -or
       -not $f.before -or -not $f.after){throw 'Only existing pinned native campaign banks permitted'}
    $pin=@($profile.files | Where-Object path -ieq $f.path)
    if($pin.Count -ne 1 -or $pin[0].sha256 -ine $f.before){throw 'Unapproved native bank baseline'}
    $expected += [pscustomobject]@{path=$f.path;before=$f.before;after=$f.after}
    $source=Resolve-GuardPath (Join-Path (Split-Path -Parent $AssetManifest) 'payload') $f.path
    $map += [pscustomobject]@{Target=$f.path;Source=$source}
}
if(@($combined.files).Count -ne $expected.Count){throw 'Unexpected combined allowance count'}
foreach($f in $expected){
    $r=@($combined.files | Where-Object path -ceq $f.path)
    if($r.Count -ne 1 -or $r[0].before -ine $f.before -or $r[0].after -ine $f.after){throw 'Combined allowance mismatch'}
}
$savedDll=Join-Path $runtime 'd3d9.dll'
$savedIni=Join-Path $runtime 'SingularityVR.ini'
$baseDll=@($profile.files | Where-Object path -ceq 'Binaries/d3d9.dll')
if($baseDll.Count -ne 1 -or (Get-GuardHash $savedDll) -ine $baseDll[0].sha256 -or
   (Get-GuardHash $savedIni) -ine $BaselineIniSha256){throw 'Wrong runtime restore destination'}
$assetScript=Join-Path $PSScriptRoot 'session-assets.ps1'
function ValidateLive {
    & $assetScript -Action ValidateLive -ManifestPath $AssetManifest -BackupDir $assets -GameRoot $game
    if(-not $?){throw 'Native bank validation failed'}
    [void](Assert-AcceptedDeployment -GameRoot $game -ProfilePath $ProfilePath -ChangePlanPath $CombinedPlan `
        -FileMap $map -Direction Install -Phase Post)
}
ValidateLive
# Freeze every transaction input and core preimage. Asset validation independently
# checks ALL payloads, protected dependencies and saved native bank preimages.
$frozen=@{}
foreach($p in @($ProfilePath,$CorePlan,$CombinedPlan,$AssetManifest,$savedDll,$savedIni,
    (Join-Path $assets 'receipt.json'),(Join-Path $candidate 'd3d9.dll'),(Join-Path $candidate 'SingularityVR.ini'))){
    $frozen[$p]=Get-GuardHash $p
    if(-not $frozen[$p]){throw 'Missing lease input'}
}
$liveDll=Join-Path $game 'Binaries/d3d9.dll'
$liveIni=Join-Path $game 'Binaries/SingularityVR.ini'
$dllHash=Get-GuardHash $liveDll
$iniHash=Get-GuardHash $liveIni
if($ValidateOnly){Write-Output 'VALIDATED: combined native-bank/runtime lease; no writes or timer';return}
Write-Output "READY: native-bank/runtime guard; ${LaunchWindowSeconds}s before launch, no active-session timeout; no launch/kill"
$deadline=(Get-Date).AddSeconds($LaunchWindowSeconds)
$seen=$false;$idle=$null
while($true){
    if(Running){$seen=$true;$idle=$null}
    elseif($seen){if(-not $idle){$idle=Get-Date};if(((Get-Date)-$idle).TotalSeconds -ge 30){break}}
    elseif((Get-Date) -ge $deadline){break}
    Start-Sleep -Seconds 2
}
if(Running){throw 'Running game: no restore writes'}
foreach($p in $frozen.Keys){if((Get-GuardHash $p) -cne $frozen[$p]){throw "Lease input drift: $p"}}
if((Get-GuardHash $liveDll) -cne $dllHash -or (Get-GuardHash $liveIni) -cne $iniHash){throw 'Live runtime drift; refusing overwrite'}
ValidateLive
# Restore bank dependencies first so core rollback can validate its frozen profile.
& (Join-Path $repo 'tools/deploy-singularityvr.ps1') -Action Rollback -SteamRoot $SteamRoot `
    -SessionAssetsManifestPath $AssetManifest -BackupPath $assets
if(-not $?){throw 'Native-bank restore failed; core preserved for review'}
& (Join-Path $repo 'tools/deploy-singularityvr.ps1') -Action Rollback -SteamRoot $SteamRoot `
    -RepoRoot $repo -BackupPath $runtime -AcceptedProfilePath $ProfilePath -ChangePlanPath $CorePlan
if(-not $?){throw 'Runtime restore failed'}
if((Get-GuardHash $liveDll) -ine $baseDll[0].sha256 -or (Get-GuardHash $liveIni) -ine $BaselineIniSha256){throw 'Runtime restore hash mismatch'}
[void](Assert-AcceptedDeployment -GameRoot $game -ProfilePath $ProfilePath)
Write-Output 'RESTORED: native banks, DLL/INI and full accepted profile independently verified'
