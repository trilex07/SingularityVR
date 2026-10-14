[CmdletBinding()]
param([Parameter(Mandatory)][string]$BackupPath,
      [ValidateRange(60,1800)][int]$LaunchWindowSeconds=1800)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$repo=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$ui='C:\Program Files (x86)\Steam\steamapps\common\Singularity\RvGame\CookedPC\UI_SinglePlayer_SF.xxx'
$dll='C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\d3d9.dll'
$candidate='69CABCCF13C0F13B120163331ADDCFDDE3764CE8E32FE60A15F800FDF8C87D66'
$acceptedDll='E3E7C90293C9BB22DFDEDE97310E86342711BDC3CBC915AF8FDCAF7BDD95D73B'
if((Get-FileHash -LiteralPath $ui).Hash -cne $candidate){throw 'Title package is not installed'}
if((Get-FileHash -LiteralPath $dll).Hash -cne $acceptedDll){throw 'DLL changed before the title window'}
Write-Output "READY: title UI restore armed; launch window $LaunchWindowSeconds seconds; DLL stays $acceptedDll"
$deadline=(Get-Date).AddSeconds($LaunchWindowSeconds)
$seen=$false
$misses=0
while($true){
    $running=@(Get-Process -Name Singularity -ErrorAction SilentlyContinue)
    if($running.Count){$seen=$true; $misses=0}
    elseif($seen){
        $misses++
        if($misses -ge 8){break}
    } elseif((Get-Date) -ge $deadline){break}
    Start-Sleep -Seconds 2
}
if(@(Get-Process -Name Singularity -ErrorAction SilentlyContinue).Count){
    Write-Output 'DEFER: game is running; title package stays installed'
    exit 0
}
if((Get-FileHash -LiteralPath $dll).Hash -cne $acceptedDll){throw 'DLL changed during the title test; UI left in place'}
if((Get-FileHash -LiteralPath $ui).Hash -cne $candidate){throw 'UI changed during the title test; refusing restore'}
& (Join-Path $repo 'tools\install-title-package.ps1') -Action Restore -BackupPath $BackupPath
if((Get-FileHash -LiteralPath $dll).Hash -cne $acceptedDll){throw 'DLL changed at UI restore'}
Write-Output 'RESTORED: accepted UI; DLL, INI and font were not written'
