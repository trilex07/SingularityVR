[CmdletBinding()]
param([Parameter(Mandatory)][string]$ManifestPath,
      [Parameter(Mandatory)][string]$BackupPath,
      [string]$SteamRoot='C:\Program Files (x86)\Steam',
      [ValidateRange(60,1800)][int]$LaunchWindowSeconds=600)
# MAIN-only four-file lease watcher. No launch, game input or process kill.
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
# A fresh hidden Windows PowerShell process can inherit a restricted module
# search path from the host. Resolve built-in modules from its own PSHOME;
# never depend on an interactive shell having imported Get-FileHash first.
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Utility') -ErrorAction Stop
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Management') -ErrorAction Stop
$deadline=(Get-Date).AddSeconds($LaunchWindowSeconds)
$seen=$false;$idleSince=$null
if(-not (Get-CimInstance Win32_Process -Filter "ProcessId=$PID" -ErrorAction Stop)){throw 'Full process view required'}
$manifestFull=(Resolve-Path -LiteralPath $ManifestPath).Path
$manifest=Get-Content -LiteralPath $manifestFull -Raw | ConvertFrom-Json
$backup=(Resolve-Path -LiteralPath $BackupPath).Path
$record=Get-Content -LiteralPath (Join-Path $backup 'lease.json') -Raw | ConvertFrom-Json
$gameRoot=[IO.Path]::GetFullPath((Join-Path $SteamRoot 'steamapps\common\Singularity'))
$allowed=@('Binaries\d3d9.dll','Binaries\dinput8.dll','Binaries\SingularityVR.ini',
           'RvGame\CookedPC\SP_VI_intro_Kismet_LOC_INT.xxx')
if($manifest.Kind -cne 'SingularityVR opening cutscene lease v1' -or @($manifest.Files).Count -ne 4 -or
   (($manifest.Files | ForEach-Object { $_.RelativePath } | Sort-Object) -join '|') -cne (($allowed | Sort-Object) -join '|') -or
   $record.GameRoot -cne $gameRoot -or $record.ManifestSha256 -cne (Get-FileHash -LiteralPath $manifestFull).Hash){throw 'Wrong restore lease'}
foreach($entry in $manifest.Files) {
    if((Get-FileHash -LiteralPath (Join-Path $backup ([IO.Path]::GetFileName($entry.RelativePath)))).Hash -cne $entry.BaselineSha256){throw 'Bad backup'}
    if((Get-FileHash -LiteralPath (Join-Path $gameRoot $entry.RelativePath)).Hash -cne $entry.CandidateSha256){throw 'Candidate not installed'}
}
Write-Output 'READY: four-file restore watcher (10min launch window by default)'
while($true) {
    if(@(Get-Process -Name Singularity -ErrorAction SilentlyContinue).Count){$seen=$true;$idleSince=$null}
    elseif($seen) {
        if(-not $idleSince){$idleSince=Get-Date}
        if(((Get-Date)-$idleSince).TotalSeconds -ge 10){break}
    } elseif((Get-Date) -ge $deadline){break}
    Start-Sleep -Seconds 2
}
& (Join-Path $PSScriptRoot 'deploy-singularityvr.ps1') -Action Rollback `
    -OpeningCutsceneManifestPath $ManifestPath -SteamRoot $SteamRoot -BackupPath $BackupPath
if(-not $?){throw 'Restore failed; inspect exact lease conflicts'}
$request=Join-Path $env:LOCALAPPDATA 'SingularityVR\hotpath-gameplay.request'
if(Test-Path -LiteralPath $request) {
    if((Get-Content -LiteralPath $request -Raw) -ceq 'SingularityVR cinema exit hotpath v1'){
        Remove-Item -LiteralPath $request
        Write-Output 'Unconsumed owned profile request removed'
    }
}
