[CmdletBinding()]
param([Parameter(Mandatory)][string]$BackupPath,
      [Parameter(Mandatory)][string]$LeaseDirectory,
      [Parameter(Mandatory)][string]$CandidateDllSha256,
      [ValidateRange(60,1800)][int]$LaunchWindowSeconds=1800)
# Waits for a normal exit, then restores the DLL backup and the one witness INI key.
# It never launches or closes the game.
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Utility') -ErrorAction Stop
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Management') -ErrorAction Stop
$repo=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$dll='C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\d3d9.dll'
$ui='C:\Program Files (x86)\Steam\steamapps\common\Singularity\RvGame\CookedPC\UI_SinglePlayer_SF.xxx'
$font='C:\Program Files (x86)\Steam\steamapps\common\Singularity\RvGame\CookedPC\ui_GFx_fonts.xxx'
$uiExpected='5B2EA469A3EE06FF154AC78BD58604E8034970D4837F289E5C22503C6A673CDE'
$fontExpected='3F7BF0BEF08D73AED3B1BA249730B4BBB16EC427DD49992A6F4C4A7CE6C8B51B'
if((Get-FileHash -LiteralPath $ui).Hash -cne $uiExpected){throw 'UI changed before the witness window'}
if((Get-FileHash -LiteralPath $font).Hash -cne $fontExpected){throw 'Font changed before the witness window'}
if((Get-FileHash -LiteralPath $dll).Hash -cne $CandidateDllSha256){throw 'Candidate DLL is not installed'}
Write-Output "READY: menu witness restore armed; launch window $LaunchWindowSeconds seconds"
$deadline=(Get-Date).AddSeconds($LaunchWindowSeconds)
$seen=$false
while($true){
    $running=@(Get-Process -Name Singularity -ErrorAction SilentlyContinue)
    if($running.Count){$seen=$true}
    elseif($seen -or (Get-Date) -ge $deadline){break}
    Start-Sleep -Seconds 2
}
if(@(Get-Process -Name Singularity -ErrorAction SilentlyContinue).Count){throw 'Game still running at restore'}
if((Get-FileHash -LiteralPath $dll).Hash -cne $CandidateDllSha256){throw 'DLL changed during the session; refusing restore'}
if((Get-FileHash -LiteralPath $ui).Hash -cne $uiExpected){throw 'UI changed during the session; refusing restore'}
if((Get-FileHash -LiteralPath $font).Hash -cne $fontExpected){throw 'Font changed during the session; refusing restore'}
& (Join-Path $repo 'tools\deploy-singularityvr.ps1') -Action Rollback -BackupPath $BackupPath -RepoRoot $repo
& (Join-Path $repo 'tools\set-menu-asset-witness.ps1') -State OFF -LeaseDirectory $LeaseDirectory -Apply
$manifest=Get-Content -LiteralPath (Join-Path $BackupPath 'backup_metadata.json') -Raw | ConvertFrom-Json
$dllEntry=@($manifest.Files | Where-Object { $_.Name -eq 'd3d9.dll' })
if((Get-FileHash -LiteralPath $dll).Hash -cne $dllEntry.Sha256){throw 'DLL rollback hash mismatch'}
if((Get-FileHash -LiteralPath $ui).Hash -cne $uiExpected -or (Get-FileHash -LiteralPath $font).Hash -cne $fontExpected){throw 'Protected package changed at restore'}
Write-Output 'RESTORED: accepted DLL and witness-free INI verified; UI and font untouched'
