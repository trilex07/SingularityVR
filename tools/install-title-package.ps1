[CmdletBinding()]
param([ValidateSet('Install','Restore')][string]$Action='Install',
      [string]$CandidatePath,
      [string]$GameRoot='C:\Program Files (x86)\Steam\steamapps\common\Singularity',
      [string]$BackupPath='')
# UI package only. The accepted renderer DLL, INI and font stay byte-identical.
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$expectedDll='D8928180262706A95972D13B728A1867F1EAB1757DCFD6C4A32D99EFBFBEC1BE'
$expectedUi='5B2EA469A3EE06FF154AC78BD58604E8034970D4837F289E5C22503C6A673CDE'
$expectedIni='202AA13A74BF1FF49104C8C507A1B9A6A6069D22DC43A529FCF6BFFC07430DBF'
$expectedFont='3F7BF0BEF08D73AED3B1BA249730B4BBB16EC427DD49992A6F4C4A7CE6C8B51B'
$expectedCandidate='E852A20D2CB613602BA3361550F44551BD7A72484CB0D834468813190E86A7CD'
function Closed {
    if(-not (Get-CimInstance Win32_Process -Filter "ProcessId=$PID" -ErrorAction Stop)){throw 'Full process view required'}
    if(@(Get-Process -Name Singularity -ErrorAction SilentlyContinue).Count){throw 'Game running; no writes'}
}
function Hash([string]$Path){(Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash}
function Child([string]$Relative){
    $base=[IO.Path]::GetFullPath($GameRoot).TrimEnd('\')
    $path=[IO.Path]::GetFullPath((Join-Path $base $Relative))
    if(-not $path.StartsWith($base+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Path escape'}
    return $path
}
$dll=Child 'Binaries\d3d9.dll'
$ini=Child 'Binaries\SingularityVR.ini'
$ui=Child 'RvGame\CookedPC\UI_SinglePlayer_SF.xxx'
$font=Child 'RvGame\CookedPC\ui_GFx_fonts.xxx'
function Assert-Protected {
    if((Hash $dll) -cne $expectedDll){throw 'DLL is not the accepted D8928180 baseline'}
    if((Hash $ini) -cne $expectedIni){throw 'INI is not the preserved user file'}
    if((Hash $font) -cne $expectedFont){throw 'Font package changed'}
}
Closed
Assert-Protected
if($Action -eq 'Install'){
    if(-not $CandidatePath){throw 'Candidate path required'}
    $candidate=(Resolve-Path -LiteralPath $CandidatePath).Path
    if((Hash $candidate) -cne $expectedCandidate){throw 'Candidate is not the reviewed title package'}
    if((Hash $ui) -cne $expectedUi){throw 'Installed UI is not the accepted original'}
    $repo=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
    $backup=Join-Path $repo ('deployment\install-backups\title-ui-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
    New-Item -ItemType Directory -Path $backup -Force | Out-Null
    Copy-Item -LiteralPath $ui -Destination (Join-Path $backup 'UI_SinglePlayer_SF.xxx') -Force
    if((Hash (Join-Path $backup 'UI_SinglePlayer_SF.xxx')) -cne $expectedUi){throw 'Backup mismatch; nothing installed'}
    @{Dll=$expectedDll;Ini=$expectedIni;Font=$expectedFont;Ui=$expectedUi;Candidate=$expectedCandidate} |
        ConvertTo-Json | Set-Content -LiteralPath (Join-Path $backup 'title-ui.json') -Encoding UTF8
    Closed
    $temp="$ui.title-$PID.new"
    Copy-Item -LiteralPath $candidate -Destination $temp -Force
    if((Hash $temp) -cne $expectedCandidate){Remove-Item -LiteralPath $temp -Force; throw 'Staged copy mismatch'}
    Move-Item -LiteralPath $temp -Destination $ui -Force
    if((Hash $ui) -cne $expectedCandidate){throw 'Installed UI hash mismatch'}
    Assert-Protected
    Write-Output "INSTALLED title UI $expectedCandidate"
    Write-Output "BACKUP $backup"
    exit 0
}
if(-not $BackupPath){throw 'Restore requires -BackupPath'}
$backup=(Resolve-Path -LiteralPath $BackupPath).Path
$saved=Join-Path $backup 'UI_SinglePlayer_SF.xxx'
if((Hash $saved) -cne $expectedUi){throw 'Backup is not the accepted UI'}
if((Hash $ui) -cne $expectedCandidate){throw 'Installed UI is no longer the title candidate; refusing restore'}
Closed
Copy-Item -LiteralPath $saved -Destination $ui -Force
if((Hash $ui) -cne $expectedUi){throw 'UI restore mismatch'}
Assert-Protected
Write-Output "RESTORED accepted UI $expectedUi"
