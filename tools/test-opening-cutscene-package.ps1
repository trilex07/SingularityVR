[CmdletBinding(SupportsShouldProcess=$true)]
param(
    [ValidateSet('Preflight','Install','Restore')][string]$Action='Preflight',
    [Parameter(Mandatory)][string]$ManifestPath,
    [string]$SteamRoot='C:\Program Files (x86)\Steam',
    [string]$BackupPath=''
)
# MAIN-only exact four-file test lease. Never launches or stops a game.
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Utility') -ErrorAction Stop
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Management') -ErrorAction Stop
$allowed=@('Binaries\d3d9.dll','Binaries\dinput8.dll','Binaries\SingularityVR.ini',
           'RvGame\CookedPC\SP_VI_intro_Kismet_LOC_INT.xxx')
$gameRoot=[IO.Path]::GetFullPath((Join-Path $SteamRoot 'steamapps\common\Singularity'))
$manifestFull=(Resolve-Path -LiteralPath $ManifestPath).Path
$manifest=Get-Content -LiteralPath $manifestFull -Raw | ConvertFrom-Json
if($manifest.Kind -cne 'SingularityVR opening cutscene lease v1'){throw 'Wrong manifest kind'}
if(@($manifest.Files).Count -ne 4){throw 'Exactly four files required'}
$actual=@($manifest.Files | ForEach-Object { $_.RelativePath } | Sort-Object)
if(($actual -join '|') -cne (($allowed | Sort-Object) -join '|')){throw 'Unexpected target set'}
function Hash([string]$Path) { (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash }
function Closed {
    if(-not (Get-CimInstance Win32_Process -Filter "ProcessId=$PID" -ErrorAction Stop)){
        throw 'Full process view required'
    }
    if(@(Get-Process -Name Singularity -ErrorAction SilentlyContinue).Count){throw 'Game is running'}
}
function Target([string]$Relative) {
    if($allowed -cnotcontains $Relative){throw 'Unknown target'}
    $path=[IO.Path]::GetFullPath((Join-Path $gameRoot $Relative))
    if(-not $path.StartsWith($gameRoot+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Path escape'}
    return $path
}
function CopyVerified([string]$Source,[string]$Destination,[string]$Expected) {
    Closed
    if((Hash $Source) -cne $Expected){throw "Source changed: $Source"}
    $stream=[IO.File]::Open($Destination,[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
    $stream.Dispose()
    $temp=$Destination+".opening-$PID.new"
    try {
        if($PSCmdlet.ShouldProcess($Destination,'Install exact verified test-lease file')) {
            Copy-Item -LiteralPath $Source -Destination $temp
            if((Hash $temp) -cne $Expected){throw 'Staged hash mismatch'}
            Closed
            Move-Item -LiteralPath $temp -Destination $Destination -Force
            if((Hash $Destination) -cne $Expected){throw 'Installed hash mismatch'}
        }
    } finally { if(Test-Path -LiteralPath $temp){Remove-Item -LiteralPath $temp} }
}
Closed
if($Action -eq 'Restore') {
    if(-not $BackupPath){throw 'Restore requires exact backup path'}
    $backup=(Resolve-Path -LiteralPath $BackupPath).Path
    $record=Get-Content -LiteralPath (Join-Path $backup 'lease.json') -Raw | ConvertFrom-Json
    if($record.GameRoot -cne $gameRoot -or $record.ManifestSha256 -cne (Hash $manifestFull)){
        throw 'Backup belongs to another install/manifest'
    }
    # Verify EVERY backup before replacing ANY installed file.
    foreach($entry in $manifest.Files) {
        $saved=Join-Path $backup ([IO.Path]::GetFileName($entry.RelativePath))
        if((Hash $saved) -cne $entry.BaselineSha256){throw 'Backup hash mismatch'}
    }
    $conflicts=@()
    foreach($entry in $manifest.Files) {
        $dst=Target $entry.RelativePath
        $current=Hash $dst
        if($current -ceq $entry.BaselineSha256){continue}
        if($current -cne $entry.CandidateSha256) {
            # Preserve unexpected user/other-owner writes while restoring the
            # remaining owned candidate files; never claim exact rollback.
            $conflicts+=$entry.RelativePath
            continue
        }
        CopyVerified (Join-Path $backup ([IO.Path]::GetFileName($entry.RelativePath))) $dst $entry.BaselineSha256
    }
    if($conflicts.Count){throw ('Unowned file changes retained: '+($conflicts -join ', '))}
    foreach($entry in $manifest.Files){if((Hash (Target $entry.RelativePath)) -cne $entry.BaselineSha256){throw 'Restore verification failed'}}
    Write-Output 'RESTORED: all four baseline hashes verified'
    return
}
foreach($entry in $manifest.Files) {
    if($entry.CandidateSha256 -cnotmatch '^[A-F0-9]{64}$' -or $entry.BaselineSha256 -cnotmatch '^[A-F0-9]{64}$'){
        throw 'Full uppercase SHA256 required'
    }
    $source=[IO.Path]::GetFullPath((Join-Path (Split-Path $manifestFull -Parent) $entry.Source))
    if((Hash $source) -cne $entry.CandidateSha256){throw "Candidate hash mismatch: $source"}
    if((Hash (Target $entry.RelativePath)) -cne $entry.BaselineSha256){throw "Installed baseline mismatch: $($entry.RelativePath)"}
    if($entry.RelativePath.EndsWith('.dll')) {
        $bytes=[IO.File]::ReadAllBytes($source)
        if($bytes.Length -lt 64 -or [BitConverter]::ToUInt16($bytes,0) -ne 0x5A4D){throw 'Not PE'}
        $pe=[BitConverter]::ToInt32($bytes,60)
        if($pe -lt 0 -or $pe+26 -gt $bytes.Length -or [BitConverter]::ToUInt32($bytes,$pe) -ne 0x4550 -or
           [BitConverter]::ToUInt16($bytes,$pe+4) -ne 0x14C -or [BitConverter]::ToUInt16($bytes,$pe+24) -ne 0x10B){throw 'Not x86 PE32'}
    }
}
# Accepted UI is read-only and never enters this lease.
if((Hash (Join-Path $gameRoot 'RvGame\CookedPC\UI_SinglePlayer_SF.xxx')) -cne $manifest.FrozenUiSha256){throw 'UI changed'}
Write-Output 'PREFLIGHT PASS: closed game, exact baseline/candidate hashes, PE32, frozen UI'
if($Action -eq 'Preflight'){return}
if($WhatIfPreference){Write-Output 'WhatIf: no backup or installation performed';return}
$root=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$backup=Join-Path $root ('deployment\hands-dubbing\backups\'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'-'+$PID)
New-Item -ItemType Directory -Path $backup | Out-Null
foreach($entry in $manifest.Files) {
    $saved=Join-Path $backup ([IO.Path]::GetFileName($entry.RelativePath))
    Copy-Item -LiteralPath (Target $entry.RelativePath) -Destination $saved
    if((Hash $saved) -cne $entry.BaselineSha256){throw 'Backup copy/hash failed'}
}
@{GameRoot=$gameRoot;ManifestSha256=(Hash $manifestFull);Created=(Get-Date).ToString('o')} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $backup 'lease.json') -Encoding UTF8
try {
    foreach($entry in $manifest.Files) {
        $src=[IO.Path]::GetFullPath((Join-Path (Split-Path $manifestFull -Parent) $entry.Source))
        CopyVerified $src (Target $entry.RelativePath) $entry.CandidateSha256
    }
    foreach($entry in $manifest.Files){if((Hash (Target $entry.RelativePath)) -cne $entry.CandidateSha256){throw 'Final installation hash mismatch'}}
} catch {
    Write-Error -ErrorAction Continue $_
    & $PSCommandPath -Action Restore -ManifestPath $manifestFull -SteamRoot $SteamRoot -BackupPath $backup
    throw 'Test installation failed; restore attempted and verified separately above'
}
Write-Output "BACKUP: $backup"
Write-Output 'INSTALLED: four candidate hashes verified; launch only through Steam'
