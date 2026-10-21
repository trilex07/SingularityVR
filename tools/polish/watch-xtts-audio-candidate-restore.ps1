[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$EvidenceDirectory,
    [Parameter(Mandatory)][string]$CandidateDirectory,
    [Parameter(Mandatory)][string]$AcceptedProfilePath,
    [string]$RuntimeBackupPath='',
    [string]$ChangePlanPath='',
    [string]$GameRoot='C:\Program Files (x86)\Steam\steamapps\common\Singularity',
    [ValidateRange(60,1800)][int]$LaunchWindowSeconds=1800
)

# MAIN-only companion lease for install-xtts-audio-candidate.ps1. It restores
# only after observing a stable game exit (or a no-launch timeout), and never
# writes while Singularity is running. Start this hidden before asking the user
# to launch the test candidate.
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Utility') -ErrorAction Stop
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Management') -ErrorAction Stop

$repo=(Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$evidence=(Resolve-Path -LiteralPath $EvidenceDirectory).Path
$candidate=(Resolve-Path -LiteralPath $CandidateDirectory).Path
$profile=(Resolve-Path -LiteralPath $AcceptedProfilePath).Path
$game=[IO.Path]::GetFullPath($GameRoot)
$leasePath=Join-Path $evidence 'xtts-audio-lease.json'
$installer=Join-Path $PSScriptRoot 'install-xtts-audio-candidate.ps1'
if(-not $evidence.StartsWith([IO.Path]::GetFullPath((Join-Path $repo 'deployment'))+'\',[StringComparison]::OrdinalIgnoreCase)){
    throw 'Evidence directory must stay under this checkout/deployment'
}
if(-not (Get-CimInstance Win32_Process -Filter "ProcessId=$PID" -ErrorAction Stop)){
    throw 'Full process visibility required for automatic XTTS restore lease'
}
$lease=Get-Content -LiteralPath $leasePath -Raw | ConvertFrom-Json
if($lease.Kind -cne 'SingularityVR XTTS audio candidate lease v1' -or
   [IO.Path]::GetFullPath($lease.GameRoot) -ine $game -or
   [IO.Path]::GetFullPath($lease.CandidateDirectory) -ine $candidate -or
   @($lease.Files).Count -ne 1770 -or
   $lease.LookupPath -cne 'Binaries/vr-assets/localization/pl/polish_story_voice.jsonl'){
    throw 'XTTS lease identity or target scope mismatch'
}
$manifest=Join-Path $candidate 'audio-payload-manifest.json'
if(-not (Test-Path -LiteralPath $profile -PathType Leaf) -or
   (Get-FileHash -LiteralPath $manifest -Algorithm SHA256).Hash.ToLowerInvariant() -cne $lease.CandidateAudioManifestSha256){
    throw 'Candidate manifest/profile missing or changed'
}
$manifestData=Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json
if((Get-FileHash -LiteralPath $profile -Algorithm SHA256).Hash.ToLowerInvariant() -cne $manifestData.acceptedProfileSha256){
    throw 'Accepted profile changed since audio lease creation'
}
$runtimeBackup=$null
$changePlan=$null
if([string]::IsNullOrWhiteSpace($RuntimeBackupPath) -xor [string]::IsNullOrWhiteSpace($ChangePlanPath)){
    throw 'RuntimeBackupPath and ChangePlanPath must be supplied together'
}
if(-not [string]::IsNullOrWhiteSpace($RuntimeBackupPath)){
    $runtimeBackup=(Resolve-Path -LiteralPath $RuntimeBackupPath).Path
    $changePlan=(Resolve-Path -LiteralPath $ChangePlanPath).Path
    $metadata=Get-Content -LiteralPath (Join-Path $runtimeBackup 'backup_metadata.json') -Raw | ConvertFrom-Json
    $names=@($metadata.Files|ForEach-Object Name|Sort-Object)
    if(($names -join ',') -cne 'd3d9.dll,SingularityVR.ini' -or @($metadata.Directories).Count -ne 0){
        throw 'Combined lease requires an exact DLL/INI-only runtime backup'
    }
    $planData=Get-Content -LiteralPath $changePlan -Raw | ConvertFrom-Json
    if($planData.schema -cne 'singularityvr-change-plan-v1' -or
       $planData.profileSha256.ToLowerInvariant() -cne (Get-FileHash -LiteralPath $profile -Algorithm SHA256).Hash.ToLowerInvariant()){
        throw 'Runtime change plan does not match the accepted profile'
    }
    $dllBackupMeta=@($metadata.Files|Where-Object{$_.Name -ceq 'd3d9.dll'})
    $iniBackupMeta=@($metadata.Files|Where-Object{$_.Name -ceq 'SingularityVR.ini'})
    $dllPlan=@($planData.files|Where-Object{$_.path -ceq 'Binaries/d3d9.dll'})
    if($dllBackupMeta.Count -ne 1 -or $iniBackupMeta.Count -ne 1 -or $dllPlan.Count -ne 1){
        throw 'Runtime backup metadata or change plan has ambiguous DLL/INI identities'
    }
    $dllBackup=Join-Path $runtimeBackup 'd3d9.dll'
    $iniBackup=Join-Path $runtimeBackup 'SingularityVR.ini'
    $dllTarget=Join-Path $game 'Binaries/d3d9.dll'
    $iniTarget=Join-Path $game 'Binaries/SingularityVR.ini'
    $dllCandidate=Join-Path $candidate 'd3d9.dll'
    $iniCandidate=Join-Path $candidate 'SingularityVR.ini'
    $dllBefore=$dllPlan[0].before.ToUpperInvariant()
    $dllAfter=$dllPlan[0].after.ToUpperInvariant()
    if((Get-FileHash -LiteralPath $dllBackup -Algorithm SHA256).Hash.ToUpperInvariant() -cne $dllBefore -or
       (Get-FileHash -LiteralPath $iniBackup -Algorithm SHA256).Hash.ToUpperInvariant() -cne $iniBackupMeta[0].Sha256.ToUpperInvariant() -or
       (Get-FileHash -LiteralPath $dllCandidate -Algorithm SHA256).Hash.ToUpperInvariant() -cne $dllAfter -or
       (Get-FileHash -LiteralPath $dllTarget -Algorithm SHA256).Hash.ToUpperInvariant() -cne $dllAfter -or
       (Get-FileHash -LiteralPath $iniTarget -Algorithm SHA256).Hash.ToUpperInvariant() -cne (Get-FileHash -LiteralPath $iniCandidate -Algorithm SHA256).Hash.ToUpperInvariant()){
        throw 'Runtime backup or installed DLL/INI does not match the combined lease'
    }
}
if(@(Get-Process -Name Singularity -ErrorAction SilentlyContinue).Count){
    throw 'Arm the XTTS restore lease before launching Singularity'
}

$deadline=(Get-Date).AddSeconds($LaunchWindowSeconds)
$seen=$false
$idleSince=$null
if($runtimeBackup){
    Write-Output "READY: combined XTTS + DLL/INI restore lease; ${LaunchWindowSeconds}s no-launch window, restore 10s after stable game exit"
}else{
    Write-Output "READY: XTTS WAV/lookup restore lease; ${LaunchWindowSeconds}s no-launch window, restore 10s after stable game exit"
}
while($true){
    $running=@(Get-Process -Name Singularity -ErrorAction SilentlyContinue)
    if($running.Count){
        $seen=$true
        $idleSince=$null
    }elseif($seen){
        if(-not $idleSince){$idleSince=Get-Date}
        if(((Get-Date)-$idleSince).TotalSeconds -ge 10){break}
    }elseif((Get-Date) -ge $deadline){
        break
    }
    Start-Sleep -Seconds 2
}
if(@(Get-Process -Name Singularity -ErrorAction SilentlyContinue).Count){
    throw 'Game still running; refusing XTTS restore'
}
& $installer -Action Rollback -CandidateDirectory $candidate -GameRoot $game `
    -AcceptedProfilePath $profile -EvidenceDirectory $evidence
if(-not $?){throw 'XTTS rollback command failed'}
Write-Output 'RESTORED: XTTS WAV and story lookup backups verified; no DLL/INI writes'
if($runtimeBackup){
    $profileData=Get-Content -LiteralPath $profile -Raw | ConvertFrom-Json
    $lookupPin=@($profileData.files|Where-Object{$_.path -ceq $lease.LookupPath})
    $lookupTarget=Join-Path $game ($lease.LookupPath -replace '/', '\')
    if($lookupPin.Count -ne 1 -or
       (Get-FileHash -LiteralPath $lookupTarget -Algorithm SHA256).Hash.ToUpperInvariant() -cne $lookupPin[0].sha256.ToUpperInvariant()){
        throw 'Story lookup did not return to the accepted profile; refusing DLL/INI rollback'
    }
    if(@(Get-Process -Name Singularity -ErrorAction SilentlyContinue).Count){
        throw 'Game relaunched after audio restore; refusing DLL/INI rollback'
    }
    & (Join-Path $repo 'tools/deploy-singularityvr.ps1') -Action Rollback `
        -BackupPath $runtimeBackup -AcceptedProfilePath $profile -ChangePlanPath $changePlan
    if(-not $?){throw 'DLL/INI rollback command failed after audio restore'}
    $metadata=Get-Content -LiteralPath (Join-Path $runtimeBackup 'backup_metadata.json') -Raw | ConvertFrom-Json
    foreach($relative in @('Binaries/d3d9.dll','Binaries/SingularityVR.ini')){
        $name=Split-Path $relative -Leaf
        $pin=@($metadata.Files|Where-Object{$_.Name -ceq $name})
        $target=Join-Path $game ($relative -replace '/', '\')
        if($pin.Count -ne 1 -or
           (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash.ToUpperInvariant() -cne $pin[0].Sha256.ToUpperInvariant()){
            throw "Post-restore runtime hash mismatch: $relative"
        }
    }
    Write-Output 'RESTORED: accepted DLL/INI hashes verified after XTTS restore'
}
