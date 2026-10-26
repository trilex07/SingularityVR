[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$CandidateDir,
    [Parameter(Mandatory)][string]$RuntimeBackup,
    [Parameter(Mandatory)][string]$AssetBackup,
    [Parameter(Mandatory)][string]$ProfilePath,
    [Parameter(Mandatory)][string]$CorePlan,
    [Parameter(Mandatory)][string]$CombinedPlan,
    [Parameter(Mandatory)][ValidatePattern('^[A-Fa-f0-9]{64}$')][string]$BaselineIniSha256,
    [string]$SteamRoot='C:\Program Files (x86)\Steam',
    [ValidateRange(60,7200)][int]$LaunchWindowSeconds=1800,
    [switch]$ValidateOnly,
    [switch]$RestoreNow
)
# MAIN-only full candidate lease. Existing production scripts own every write.
# No launch/kill. No timeout while a game session is active. RestoreNow is for
# explicit MAIN recovery/testing after exit; it still performs every preflight.
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
. (Join-Path $repo 'tools/deployment/AcceptedProfileGuard.ps1')
. (Join-Path $PSScriptRoot 'session-asset-scope.ps1')
$game=Join-Path $SteamRoot 'steamapps/common/Singularity'
$manifestPath=Join-Path $CandidateDir 'manifest.json'
$runtimeDir=Join-Path $CandidateDir 'runtime'
foreach($path in @($CandidateDir,$AssetBackup,$ProfilePath,$CorePlan,$CombinedPlan)){
    $full=[IO.Path]::GetFullPath($path)
    if(-not $full.StartsWith($repo+'\build\next-weapon-reload\',[StringComparison]::OrdinalIgnoreCase)){
        throw 'Final session inputs must belong to this checkout private build root'
    }
}
if(-not ([IO.Path]::GetFullPath($RuntimeBackup)).StartsWith($repo+'\deployment\install-backups\',[StringComparison]::OrdinalIgnoreCase)){
    throw 'Runtime backup must belong to this checkout'
}
function Running {
    [void](Get-Process -Id $PID -ErrorAction Stop)
    return @(Get-Process -Name Singularity -ErrorAction SilentlyContinue).Count -gt 0
}
function SameHash($A,$B) {
    return [string]::Equals([string]$A,[string]$B,[StringComparison]::OrdinalIgnoreCase)
}
$manifest=Read-GuardJson $manifestPath
$profile=Read-GuardJson $ProfilePath
$core=Read-GuardJson $CorePlan
$combined=Read-GuardJson $CombinedPlan
if($manifest.schema -cne 'singularityvr-session-assets-delta-v1' -or
   $core.profileSha256 -ine (Get-GuardHash $ProfilePath) -or
   $combined.profileSha256 -ine $core.profileSha256 -or
   ($combined.ini|ConvertTo-Json -Depth 8 -Compress) -cne ($core.ini|ConvertTo-Json -Depth 8 -Compress)){
    throw 'Unbound combined candidate plans'
}
$names=@('d3d9.dll','dinput8.dll','SingularityVR.ini','texture_override.ini')
$metadata=Read-GuardJson (Join-Path $RuntimeBackup 'backup_metadata.json')
if(@($metadata.Directories).Count -or @($metadata.Files).Count -ne 4){throw 'Unexpected runtime backup inventory'}
foreach($name in $names){
    $entry=@($metadata.Files|Where-Object Name -ceq $name)
    if($entry.Count -ne 1 -or -not $entry[0].Existed){throw 'Missing accepted runtime backup'}
}
$map=@();$expected=@($core.files);$frozen=@{}
foreach($name in $names){
    $map += [pscustomobject]@{Target='Binaries/'+$name;Source=(Join-Path $runtimeDir $name)}
    $saved=Join-Path $RuntimeBackup $name
    $expectedHash=if($name -ceq 'SingularityVR.ini'){$BaselineIniSha256}else{
        $pin=@($profile.files|Where-Object path -ceq ('Binaries/'+$name))
        if($pin.Count -ne 1){throw 'Missing runtime baseline pin'}
        $pin[0].sha256
    }
    if((Get-GuardHash $saved) -ine $expectedHash){throw "Wrong runtime restore preimage: $name"}
    $frozen[$saved]=Get-GuardHash $saved
}
foreach($row in $manifest.files){
    if(-not (Test-SessionAssetPath $row.path)){throw 'Asset outside reviewed lease scope'}
    $pin=@($profile.files|Where-Object path -ceq $row.path)
    if($pin.Count -ne 1 -or -not (SameHash $pin[0].sha256 $row.before)){
        throw "Asset baseline not pinned: $($row.path)"
    }
    if(-not (SameHash $row.before $row.after)){
        $expected += [pscustomobject]@{path=$row.path;before=$row.before;after=$row.after}
    }
    $map += [pscustomobject]@{Target=$row.path;Source=(Resolve-GuardPath (Join-Path $CandidateDir 'payload') $row.path)}
}
if(@($combined.files).Count -ne $expected.Count){throw 'Unexpected combined allowance count'}
foreach($row in $expected){
    $entry=@($combined.files|Where-Object path -ceq $row.path)
    if($entry.Count -ne 1 -or -not (SameHash $entry[0].before $row.before) -or
       -not (SameHash $entry[0].after $row.after)){throw 'Combined allowance mismatch'}
}
$assetScript=Join-Path $PSScriptRoot 'session-assets.ps1'
function ValidateLive {
    & $assetScript -Action ValidateLive -ManifestPath $manifestPath -BackupDir $AssetBackup -GameRoot $game
    if(-not $?){throw 'Combined asset validation failed'}
    [void](Assert-AcceptedDeployment -GameRoot $game -ProfilePath $ProfilePath -ChangePlanPath $CombinedPlan `
        -FileMap $map -Direction Install -Phase Post)
}
ValidateLive
foreach($path in @($ProfilePath,$CorePlan,$CombinedPlan,$manifestPath,
    (Join-Path $AssetBackup 'receipt.json'),(Join-Path $RuntimeBackup 'manifest.json'),
    (Join-Path $RuntimeBackup 'backup_metadata.json')) + @($map|ForEach-Object Source)){
    $hash=Get-GuardHash $path
    if(-not $hash){throw "Missing lease input: $path"}
    $frozen[$path]=$hash
}
if($ValidateOnly){Write-Output 'VALIDATED: four runtime files and all asset inputs; no writes or timer';return}
if(-not $RestoreNow){
    Write-Output "READY: final session guard; ${LaunchWindowSeconds}s BEFORE launch; no active-session timeout"
    $deadline=(Get-Date).AddSeconds($LaunchWindowSeconds);$seen=$false;$idle=$null
    while($true){
        if(Running){$seen=$true;$idle=$null}
        elseif($seen){if(-not $idle){$idle=Get-Date};if(((Get-Date)-$idle).TotalSeconds -ge 30){break}}
        elseif((Get-Date) -ge $deadline){break}
        Start-Sleep -Seconds 2
    }
}
if(Running){throw 'Running game: no restore writes'}
foreach($path in $frozen.Keys){if((Get-GuardHash $path) -cne $frozen[$path]){throw "Lease input drift: $path"}}
ValidateLive
# Prove the complete core backup/rollback map BEFORE restoring even one asset.
$savedMap=@($names|ForEach-Object {[pscustomobject]@{Target='Binaries/'+$_;Source=(Join-Path $RuntimeBackup $_)}})
$rollbackTicket=Assert-AcceptedDeployment -GameRoot $game -ProfilePath $ProfilePath `
    -ChangePlanPath $CombinedPlan -FileMap (@($savedMap)+@($manifest.files|ForEach-Object {
        [pscustomobject]@{Target=$_.path;Source=if($_.before){Resolve-GuardPath $AssetBackup $_.path}else{$null}}
    })) -Direction Rollback -Phase Pre
& (Join-Path $repo 'tools/deploy-singularityvr.ps1') -Action Rollback -SteamRoot $SteamRoot `
    -SessionAssetsManifestPath $manifestPath -BackupPath $AssetBackup
if(-not $?){throw 'Asset restore failed; runtime preserved'}
& (Join-Path $repo 'tools/deploy-singularityvr.ps1') -Action Rollback -SteamRoot $SteamRoot `
    -RepoRoot $repo -BackupPath $RuntimeBackup -AcceptedProfilePath $ProfilePath -ChangePlanPath $CorePlan
if(-not $?){throw 'Runtime restore failed'}
if((Get-GuardHash (Join-Path $game 'Binaries/SingularityVR.ini')) -ine $BaselineIniSha256){throw 'INI exact restore mismatch'}
[void](Assert-AcceptedDeployment -GameRoot $game -ProfilePath $ProfilePath)
Write-Output 'RESTORED: accepted four-file runtime and complete asset baseline; hashes verified'
