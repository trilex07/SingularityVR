[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$BackupPath,
    [Parameter(Mandatory)][string]$CandidateDir,
    [Parameter(Mandatory)][string]$AssetBackupDir,
    [Parameter(Mandatory)][string]$AcceptedProfilePath,
    [Parameter(Mandatory)][string]$ChangePlanPath,
    [string]$NpcCandidateDir,
    [string]$NpcBackupDir,
    [string]$SessionAssetsManifestPath,
    [string]$SessionAssetsBackupDir,
    [string]$SteamRoot='C:\Program Files (x86)\Steam',
    [ValidateRange(60,7200)][int]$LaunchWindowSeconds=600
)

# Bounded MAIN test lease for the runtime DLL/INI and nine reload assets. Never
# launches, terminates, or writes while the game is running. Refuses drift.
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$backup=[IO.Path]::GetFullPath($BackupPath)
$candidate=[IO.Path]::GetFullPath($CandidateDir)
$assetBackup=[IO.Path]::GetFullPath($AssetBackupDir)
$profile=[IO.Path]::GetFullPath($AcceptedProfilePath)
$plan=[IO.Path]::GetFullPath($ChangePlanPath)
$buildRoot=[IO.Path]::GetFullPath((Join-Path $repo 'build\next-weapon-reload')).TrimEnd('\')+'\'
$backupRoot=[IO.Path]::GetFullPath((Join-Path $repo 'deployment\install-backups')).TrimEnd('\')+'\'
if(-not $candidate.StartsWith($buildRoot,[StringComparison]::OrdinalIgnoreCase) -or
   -not $assetBackup.StartsWith($buildRoot,[StringComparison]::OrdinalIgnoreCase) -or
   -not $backup.StartsWith($backupRoot,[StringComparison]::OrdinalIgnoreCase)){
    throw 'Runtime lease paths must be inside this checkout build/backup roots'
}
$steam=[IO.Path]::GetFullPath($SteamRoot)
$gameRoot=Join-Path $steam 'steamapps\common\Singularity'
$gameBin=Join-Path $gameRoot 'Binaries'
$liveDll=Join-Path $gameBin 'd3d9.dll'
$liveIni=Join-Path $gameBin 'SingularityVR.ini'
$candidateDll=Join-Path $candidate 'd3d9.dll'
$candidateIni=Join-Path $candidate 'SingularityVR.ini'
$savedDll=Join-Path $backup 'd3d9.dll'
$savedIni=Join-Path $backup 'SingularityVR.ini'
foreach($path in @($liveDll,$liveIni,$candidateDll,$candidateIni,$savedDll,$savedIni,$profile,$plan)){
    if(-not (Test-Path -LiteralPath $path -PathType Leaf)){throw "Missing runtime lease file: $path"}
}
if(-not (Get-CimInstance Win32_Process -Filter "ProcessId=$PID" -ErrorAction Stop)){
    throw 'Full process visibility required for runtime restore lease'
}
function Hash([string]$path){
    if(-not (Test-Path -LiteralPath $path -PathType Leaf)){return $null}
    return (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToUpperInvariant()
}
$dllHash=Hash $candidateDll
$iniHash=Hash $candidateIni
$backupDllHash=Hash $savedDll
$backupIniHash=Hash $savedIni
$profileHash=Hash $profile
$planHash=Hash $plan
$assetReceiptPath=Join-Path $assetBackup 'assets.json'
if(-not (Test-Path -LiteralPath $assetReceiptPath -PathType Leaf)){throw 'Missing reload asset backup receipt'}
$assetReceiptHash=Hash $assetReceiptPath
$manifestPath=Join-Path $candidate 'manifest.json'
$assetManifestHash=Hash $manifestPath
$manifest=Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
$assetReceipt=Get-Content -Raw -LiteralPath $assetReceiptPath | ConvertFrom-Json
$expectedAssets=@(
    'vr-assets/reload/valkyrie/valkyrie_reload.svwa',
    'vr-assets/reload/valkyrie/valkyrie_clip.svrk',
    'vr-assets/reload/valkyrie/valkyrie_clip.svrt',
    'vr-assets/reload/volk/volk_reload.svwa',
    'vr-assets/reload/volk/volk_clip.svrk',
    'vr-assets/reload/volk/volk_clip.svrt',
    'vr-assets/reload/kasimov/kasimov_reload.svwa',
    'vr-assets/reload/kasimov/kasimov_clip.svrk',
    'vr-assets/reload/kasimov/kasimov_clip.svrt'
)
if(@($assetReceipt).Count -ne 9){throw 'Reload asset receipt must list exactly nine files'}
foreach($relative in $expectedAssets){
    $entries=@($assetReceipt | Where-Object path -ceq $relative)
    $listed=@($manifest.files | Where-Object path -ceq $relative)
    $installed=Join-Path $gameBin $relative.Replace('/','\')
    if($entries.Count -ne 1 -or $listed.Count -ne 1 -or
       $entries[0].after.ToUpperInvariant() -cne $listed[0].sha256.ToUpperInvariant() -or
       (Hash $installed) -cne $entries[0].after.ToUpperInvariant()){
        throw "Reload asset does not match candidate/receipt: $relative"
    }
}
$dllEntry=@($manifest.files | Where-Object path -ceq 'd3d9.dll')
$build=Get-Content -Raw -LiteralPath (Join-Path $candidate 'd3d9.build.json') | ConvertFrom-Json
if($dllEntry.Count -ne 1 -or $dllEntry[0].sha256.ToUpperInvariant() -cne $dllHash -or
   $build.sha256.ToUpperInvariant() -cne $dllHash -or $build.architecture -cne 'x86 PE32' -or
   $build.optimization -cne '/O2' -or @($build.workingTreeStatus).Count){
    throw 'Runtime DLL does not match the clean x86 /O2 candidate receipt'
}
if((Hash $liveDll) -cne $dllHash -or (Hash $liveIni) -cne $iniHash){
    throw 'Installed DLL/INI do not match candidate; refusing lease'
}
if($backupDllHash -cne '789749E5D4DB8FC6C603DF599A38A4FB1EC00DE374E4E2FDD469B09B9C5F361B' -or
   $backupIniHash -cne '8B0B0B4CF2DB78514A44B72CF3E79CC1C48C74E5632CDA34A468CA61702CCFAF'){
    throw 'Saved DLL/INI are not the independently verified accepted baseline'
}
$ui=Join-Path $gameRoot 'RvGame\CookedPC\UI_SinglePlayer_SF.xxx'
$uiHash=if(Test-Path -LiteralPath $ui -PathType Leaf){Hash $ui}else{$null}
. (Join-Path $repo 'tools\deployment\AcceptedProfileGuard.ps1')
$fileMap=@(
    [pscustomobject]@{Target='Binaries/d3d9.dll';Source=$candidateDll},
    [pscustomobject]@{Target='Binaries/SingularityVR.ini';Source=$candidateIni}
)
$ticket=Assert-AcceptedDeployment -GameRoot $gameRoot -ProfilePath $profile -ChangePlanPath $plan `
    -FileMap $fileMap -Direction Install -Phase Post
if($ticket.ProfileSha256 -cne $profileHash){throw 'Accepted profile changed during preflight'}
if([bool]$NpcCandidateDir -ne [bool]$NpcBackupDir){throw 'NPC lease requires both candidate and backup paths'}
if($NpcCandidateDir){
    & (Join-Path $repo 'tools/title/deploy_npc_name_test.ps1') -Action ValidateLive `
        -CandidateDir $NpcCandidateDir -BackupDir $NpcBackupDir -SteamRoot $steam
    if(-not $?){throw 'NPC package lease preflight failed'}
}

if($SessionAssetsManifestPath){
    & (Join-Path $repo 'tools/integration/session-assets.ps1') -Action ValidateLive `
        -ManifestPath $SessionAssetsManifestPath -BackupDir $SessionAssetsBackupDir -GameRoot $gameRoot
    if(-not $?){throw 'Combined session asset preflight failed'}
}
Write-Output "READY: all-file lease armed (${LaunchWindowSeconds}s BEFORE launch only); active sessions have NO timeout; no launch/kill."
Write-Output "Candidate DLL=$dllHash INI=$iniHash; nine reload assets verified; protected UI SHA256=$uiHash"
$deadline=(Get-Date).AddSeconds($LaunchWindowSeconds)
$seen=$false
$idle=$null
while($true){
    $running=@(Get-CimInstance Win32_Process -Filter "Name='Singularity.exe'" -ErrorAction Stop)
    if($running.Count){$seen=$true;$idle=$null}
    elseif($seen){
        if(-not $idle){$idle=Get-Date}
        if(((Get-Date)-$idle).TotalSeconds -ge 30){break}
    }
    elseif((Get-Date) -ge $deadline){break}
    Start-Sleep -Seconds 2
}
if(@(Get-CimInstance Win32_Process -Filter "Name='Singularity.exe'" -ErrorAction Stop).Count){
    throw 'Game still running; refusing to restore files in use'
}
if((Hash $profile) -cne $profileHash -or (Hash $plan) -cne $planHash -or
   (Hash $manifestPath) -cne $assetManifestHash -or (Hash $assetReceiptPath) -cne $assetReceiptHash){
    throw 'Profile/plan/asset receipt changed during lease; preserve candidate for manual review'
}
if((Hash $liveDll) -cne $dllHash -or (Hash $liveIni) -cne $iniHash){
    throw 'Installed runtime drifted during test; refusing overwrite'
}
foreach($entry in $assetReceipt){
    $assetPath=Join-Path $gameBin ([string]$entry.path).Replace('/','\')
    if((Hash $assetPath) -cne $entry.after.ToUpperInvariant()){
        throw "Installed reload asset drifted; refusing overwrite: $($entry.path)"
    }
    if($entry.before -and (Hash (Join-Path $assetBackup ([string]$entry.path).Replace('/','\'))) -cne $entry.before.ToUpperInvariant()){
        throw "Reload asset backup drifted: $($entry.path)"
    }
}
if($uiHash -and (Hash $ui) -cne $uiHash){throw 'Protected UI package changed during test'}
if($NpcCandidateDir){
    & (Join-Path $repo 'tools/title/deploy_npc_name_test.ps1') -Action ValidateLive `
        -CandidateDir $NpcCandidateDir -BackupDir $NpcBackupDir -SteamRoot $steam
    if(-not $?){throw 'NPC package drift during lease'}
}

if($SessionAssetsManifestPath){
    & (Join-Path $repo 'tools\deploy-singularityvr.ps1') -Action Rollback -SteamRoot $steam `
        -SessionAssetsManifestPath $SessionAssetsManifestPath -BackupPath $SessionAssetsBackupDir
    if(-not $?){throw 'Combined session asset restore failed'}
}
& (Join-Path $repo 'tools\deploy-singularityvr.ps1') -Action Rollback `
    -SteamRoot $steam -RepoRoot $repo -BackupPath $backup -AcceptedProfilePath $profile `
    -ChangePlanPath $plan
if(-not $?){throw 'Accepted runtime rollback failed'}
if((Hash $liveDll) -cne $backupDllHash -or (Hash $liveIni) -cne $backupIniHash){
    throw 'Restored runtime DLL/INI do not match verified backup hashes'
}
& (Join-Path $PSScriptRoot 'deploy_next_weapon_test.ps1') -Action Restore `
    -GameBin $gameBin -BackupDir $assetBackup
if(-not $?){throw 'Reload asset rollback failed'}
foreach($entry in $assetReceipt){
    $assetPath=Join-Path $gameBin ([string]$entry.path).Replace('/','\')
    if((Hash $assetPath) -cne $entry.before){throw "Restored reload asset differs from baseline: $($entry.path)"}
}
if($uiHash -and (Hash $ui) -cne $uiHash){throw 'Protected UI package changed during rollback'}
if($NpcCandidateDir){
    & (Join-Path $repo 'tools/title/deploy_npc_name_test.ps1') -Action Restore `
        -CandidateDir $NpcCandidateDir -BackupDir $NpcBackupDir -SteamRoot $steam
    if(-not $?){throw 'NPC package rollback failed'}
}
$phase=if($seen){'test session ended'}else{'no session launched before timeout'}
Write-Output "RESTORED: accepted DLL/INI and all nine reload asset states verified ($phase); UI package untouched."
