[CmdletBinding()]
param(
 [Parameter(Mandatory)][string]$BackupPath,
 [Parameter(Mandatory)][string]$AcceptedProfilePath,
 [Parameter(Mandatory)][string]$ChangePlanPath,
 [string]$SteamRoot='C:\Program Files (x86)\Steam',
 [ValidateRange(60,1800)][int]$LaunchWindowSeconds=600
)
# MAIN's DLL/INI lease only. The exact two cursor packages have their own
# disjoint asset lease. No launch, input, registry changes or process termination.
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Utility')
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Management')
. (Join-Path $PSScriptRoot 'deployment/AcceptedProfileGuard.ps1')
if(-not(Get-CimInstance Win32_Process -Filter "ProcessId=$PID" -ErrorAction Stop)){throw 'Full process visibility required'}
$backup=(Resolve-Path -LiteralPath $BackupPath).Path
$profile=(Resolve-Path -LiteralPath $AcceptedProfilePath).Path
$plan=(Resolve-Path -LiteralPath $ChangePlanPath).Path
$metadata=Read-GuardJson (Join-Path $backup 'backup_metadata.json')
$names=@($metadata.Files|ForEach-Object Name|Sort-Object)
if(($names -join ',') -cne 'd3d9.dll,SingularityVR.ini' -or @($metadata.Directories).Count){throw 'Exactly DLL and INI backup required'}
$gameRoot=Join-Path $SteamRoot 'steamapps\common\Singularity'
$map=@()
foreach($entry in $metadata.Files){
 if(-not $entry.Existed){throw 'Both baseline files must exist'}
 $source=Join-Path $backup $entry.Name
 if((Get-GuardHash $source) -cne $entry.Sha256){throw 'Backup hash mismatch'}
 $map += [pscustomobject]@{Target=('Binaries/'+$entry.Name);Source=$source}
}
$ticket=Assert-AcceptedDeployment -GameRoot $gameRoot -ProfilePath $profile -ChangePlanPath $plan -FileMap $map -Direction Rollback
Write-Output "READY: exact DLL/INI menu lease; ${LaunchWindowSeconds}s no-launch window, restore 10s after exit"
$deadline=(Get-Date).AddSeconds($LaunchWindowSeconds);$seen=$false;$idle=$null
while($true){
 if(@(Get-CimInstance Win32_Process -Filter "Name='Singularity.exe'" -ErrorAction Stop).Count){$seen=$true;$idle=$null}
 elseif($seen){if(-not $idle){$idle=Get-Date};if(((Get-Date)-$idle).TotalSeconds -ge 10){break}}
 elseif((Get-Date) -ge $deadline){break}
 Start-Sleep -Seconds 2
}
if((Get-GuardHash $profile) -cne $ticket.ProfileSha256 -or (Get-GuardHash $plan) -cne $ticket.PlanSha256){throw 'Lease identity changed'}
foreach($entry in $metadata.Files){if((Get-GuardHash (Join-Path $backup $entry.Name)) -cne $entry.Sha256){throw 'Backup changed during test'}}
& (Join-Path $PSScriptRoot 'deploy-singularityvr.ps1') -Action Rollback -BackupPath $backup -SteamRoot $SteamRoot -AcceptedProfilePath $profile -ChangePlanPath $plan
if(-not $?){throw 'Guarded menu rollback failed'}
foreach($entry in $metadata.Files){
 if((Get-GuardHash (Join-Path $gameRoot ('Binaries/'+$entry.Name))) -cne $entry.Sha256){throw 'Post-restore mismatch'}
}
Write-Output 'RESTORED: exact pre-test DLL/INI verified; independent cursor assets not touched'
