[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$EvidenceDirectory,
    [Parameter(Mandatory)][string]$CandidateDirectory,
    [Parameter(Mandatory)][string]$AcceptedProfilePath,
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
if(@(Get-Process -Name Singularity -ErrorAction SilentlyContinue).Count){
    throw 'Arm the XTTS restore lease before launching Singularity'
}

$deadline=(Get-Date).AddSeconds($LaunchWindowSeconds)
$seen=$false
$idleSince=$null
Write-Output "READY: XTTS WAV/lookup restore lease; ${LaunchWindowSeconds}s no-launch window, restore 10s after stable game exit"
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
