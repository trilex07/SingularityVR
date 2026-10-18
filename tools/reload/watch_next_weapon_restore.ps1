[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$GameBin,
    [Parameter(Mandatory)][string]$BackupDir,
    [Parameter(Mandatory)][string]$CandidateRoot,
    [ValidateRange(60,1800)][int]$LaunchWindowSeconds=600
)
# Independent nine-file asset lease. No launch, input or process termination.
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Utility') -ErrorAction Stop
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Management') -ErrorAction Stop
if(-not (Get-CimInstance Win32_Process -Filter "ProcessId=$PID" -ErrorAction Stop)){
    throw 'Full process visibility required'
}
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$bin=[IO.Path]::GetFullPath($GameBin)
$backup=[IO.Path]::GetFullPath($BackupDir)
$candidate=[IO.Path]::GetFullPath($CandidateRoot)
if(-not $backup.StartsWith((Join-Path $repo 'build').TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase) -or
   -not $candidate.StartsWith((Join-Path $repo 'build').TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase) -or
   (Split-Path $bin -Leaf) -ine 'Binaries' -or
   -not (Test-Path -LiteralPath (Join-Path $bin 'Singularity.exe') -PathType Leaf)){
    throw 'Unexpected asset-lease paths'
}
function Hash([string]$path){
    if(-not (Test-Path -LiteralPath $path -PathType Leaf)){return $null}
    return (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToUpperInvariant()
}
$manifestPath=Join-Path $candidate 'manifest.json'
$receiptPath=Join-Path $backup 'assets.json'
$manifestHash=Hash $manifestPath
$receiptHash=Hash $receiptPath
if(-not $manifestHash -or -not $receiptHash){throw 'Missing asset manifest or backup receipt'}
$manifest=Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
$receipt=Get-Content -Raw -LiteralPath $receiptPath | ConvertFrom-Json
if($receipt.Count -ne 9){throw 'Expected nine exact reload assets'}
foreach($entry in $receipt){
    $rel=[string]$entry.path
    if($rel -cnotmatch '^vr-assets/reload/(valkyrie|volk|kasimov)/\1_(clip\.svrk|clip\.svrt|reload\.svwa)$'){
        throw "Unlisted asset: $rel"
    }
    $listed=@($manifest.files | Where-Object path -ceq $rel)
    if($listed.Count -ne 1 -or $entry.after -cne $listed[0].sha256.ToUpperInvariant() -or
       (Hash (Join-Path $bin $rel)) -cne $entry.after){
        throw "Candidate asset not installed as planned: $rel"
    }
    if($entry.before -and (Hash (Join-Path $backup $rel)) -cne $entry.before){
        throw "Bad asset backup: $rel"
    }
}
Write-Output "READY: nine-file reload asset lease; ${LaunchWindowSeconds}s no-launch window, restore 10s after exit"
$deadline=(Get-Date).AddSeconds($LaunchWindowSeconds);$seen=$false;$idle=$null
while($true){
    if(@(Get-CimInstance Win32_Process -Filter "Name='Singularity.exe'" -ErrorAction Stop).Count){
        $seen=$true;$idle=$null
    }elseif($seen){
        if(-not $idle){$idle=Get-Date}
        if(((Get-Date)-$idle).TotalSeconds -ge 10){break}
    }elseif((Get-Date) -ge $deadline){break}
    Start-Sleep -Seconds 2
}
if((Hash $manifestPath) -cne $manifestHash -or (Hash $receiptPath) -cne $receiptHash){
    throw 'Asset lease identity changed'
}
& (Join-Path $PSScriptRoot 'deploy_next_weapon_test.ps1') -Action Restore -GameBin $bin -BackupDir $backup
if(-not $?){throw 'Reload asset restore failed'}
Write-Output 'RESTORED: nine reload assets match their pre-test hashes/absence'
