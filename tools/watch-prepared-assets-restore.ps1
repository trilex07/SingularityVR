[CmdletBinding()]
param([Parameter(Mandatory)][string]$ManifestPath,[Parameter(Mandatory)][string]$BackupPath,
 [string]$GameRoot='C:\Program Files (x86)\Steam\steamapps\common\Singularity',
 [ValidateRange(60,1800)][int]$LaunchWindowSeconds=600)
# MAIN independent delta guard. No launch/input/kill; no core/WAV writes.
Set-StrictMode -Version Latest;$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Utility')
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Management')
if(-not(Get-CimInstance Win32_Process -Filter "ProcessId=$PID")){throw 'Full process view required'}
$manifest=Get-Content $ManifestPath -Raw | ConvertFrom-Json
$record=Get-Content (Join-Path $BackupPath 'lease.json') -Raw | ConvertFrom-Json
$profileOk=($manifest.Kind -ceq 'SingularityVR prepared asset delta v1' -and @($manifest.Files).Count -eq 32) -or
 ($manifest.Kind -ceq 'SingularityVR post-opening asset delta v2' -and @($manifest.Files).Count -eq 34)
if(-not $profileOk -or
 $record.GameRoot -cne [IO.Path]::GetFullPath($GameRoot) -or $record.ManifestSha256 -cne (Get-FileHash $ManifestPath).Hash){throw 'Wrong delta guard lease'}
foreach($f in $manifest.Files){
 if((Get-FileHash (Join-Path $GameRoot $f.RelativePath)).Hash -cne $f.CandidateSha256){throw 'Candidate not installed'}
 if($f.BaselineSha256 -cne 'ABSENT' -and (Get-FileHash (Join-Path $BackupPath $f.RelativePath)).Hash -cne $f.BaselineSha256){throw 'Bad guard backup'}
}
Write-Output ("READY: independent$(@($manifest.Files).Count)-file asset guard, bounded${LaunchWindowSeconds}s no-launch/10s after exit")
$deadline=(Get-Date).AddSeconds($LaunchWindowSeconds);$seen=$false;$idle=$null
while($true){
 if(@(Get-Process -Name Singularity -ErrorAction SilentlyContinue).Count){$seen=$true;$idle=$null}
 elseif($seen){if(-not $idle){$idle=Get-Date};if(((Get-Date)-$idle).TotalSeconds -ge 10){break}}
 elseif((Get-Date) -ge $deadline){break}
 Start-Sleep -Seconds 2
}
& (Join-Path $PSScriptRoot 'test-prepared-assets.ps1') -Action Restore -ManifestPath $ManifestPath -GameRoot $GameRoot -BackupPath $BackupPath
if(-not $?){throw 'Delta restore failed; preserve conflict evidence'}
