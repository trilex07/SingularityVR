[CmdletBinding()]
param([Parameter(Mandatory)][string]$EvidenceDirectory,
      [ValidateRange(60,600)][int]$LaunchWindowSeconds=600)
Import-Module (Join-Path $PSHOME 'Modules/Microsoft.PowerShell.Utility/Microsoft.PowerShell.Utility.psd1')
Import-Module (Join-Path $PSHOME 'Modules/Microsoft.PowerShell.Management/Microsoft.PowerShell.Management.psd1')
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
if(-not(Get-CimInstance Win32_Process -Filter "ProcessId=$PID")){throw 'Full process visibility required'}
$plan=Get-Content (Join-Path $EvidenceDirectory 'revoice-lease.json') -Raw|ConvertFrom-Json
if($plan.Kind -cne 'SingularityVR revoice WAV lease v1' -or @($plan.Files).Count -ne 1886){throw 'Wrong voice lease'}
if(@(Get-Process -Name Singularity -ErrorAction SilentlyContinue).Count){throw 'Must arm before launch'}
foreach($f in $plan.Files){
    if($f.RelativePath -notmatch '^Binaries\\vr-assets\\audio\\polish\\(ui|story)\\[a-zA-Z0-9_-]+\.wav$' -or $f.Backup -notmatch '^backup\\(ui|story)\\[a-zA-Z0-9_-]+\.wav$'){throw 'Invalid WAV scope'}
    if((Get-FileHash (Join-Path $EvidenceDirectory $f.Backup)).Hash -cne $f.BaselineSha256){throw 'Invalid voice backup'}
    if((Get-FileHash (Join-Path $plan.GameRoot $f.RelativePath)).Hash -cne $f.CandidateSha256){throw 'Candidate voice hash mismatch'}
}
$deadline=(Get-Date).AddSeconds($LaunchWindowSeconds)
$seen=$false;$absent=$null
Write-Output ('READY: WAV-only guardian, launch window '+$LaunchWindowSeconds+'s, restore after exit/no launch')
while($true){
    $game=@(Get-Process -Name Singularity -ErrorAction SilentlyContinue)
    if($game.Count){$seen=$true;$absent=$null}
    elseif($seen){
        if(-not $absent){$absent=Get-Date}
        if(((Get-Date)-$absent).TotalSeconds -ge 10){break}
    }elseif((Get-Date) -ge $deadline){break}
    Start-Sleep -Seconds 2
}
& (Join-Path $PSScriptRoot 'install-revoice-candidate.ps1') -Action Rollback -GameRoot $plan.GameRoot -EvidenceDirectory $EvidenceDirectory
Write-Output 'GUARD FINISHED: WAV originals restored; no DLL/INI/package writes'
