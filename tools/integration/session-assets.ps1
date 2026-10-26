[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidateSet('ValidateCandidate','Preflight','Deploy','ValidateLive','Restore')][string]$Action,
    [Parameter(Mandatory)][string]$ManifestPath,
    [Parameter(Mandatory)][string]$BackupDir,
    [string]$GameRoot='C:\Program Files (x86)\Steam\steamapps\common\Singularity'
)
# MAIN-only asset delta, called through deploy-singularityvr.ps1. No launch,
# timer, DLL/config/MP/UI-audio ownership, recursive deletion or broad restore.
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'session-asset-scope.ps1')
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$allowed=[IO.Path]::GetFullPath((Join-Path $repo 'build\next-weapon-reload')).TrimEnd('\')+'\'
$manifestPath=[IO.Path]::GetFullPath($ManifestPath)
$backup=[IO.Path]::GetFullPath($BackupDir)
if(-not $manifestPath.StartsWith($allowed,[StringComparison]::OrdinalIgnoreCase) -or
   -not $backup.StartsWith($allowed,[StringComparison]::OrdinalIgnoreCase)){throw 'Asset manifest and backup must be in this checkout private build root'}
$candidate=Split-Path -Parent $manifestPath
$game=[IO.Path]::GetFullPath($GameRoot).TrimEnd('\')
function Hash([string]$path){
    if(-not (Test-Path -LiteralPath $path -PathType Leaf)){return $null}
    return (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
}
function Under([string]$root,[string]$relative){
    $p=[IO.Path]::GetFullPath((Join-Path $root $relative.Replace('/','\')))
    if(-not $p.StartsWith($root.TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Path escape'}
    $walk=$p
    while($walk -and $walk.Length -ge $root.Length){
        if((Test-Path -LiteralPath $walk) -and ((Get-Item -LiteralPath $walk -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)){
            throw "Reparse point in asset lease: $walk"
        }
        if($walk -ceq $root){break}
        $walk=Split-Path -Parent $walk
    }
    return $p
}
function Stopped {
    if(-not (Get-CimInstance Win32_Process -Filter "ProcessId=$PID" -ErrorAction Stop)){throw 'Full process visibility required'}
    if(@(Get-CimInstance Win32_Process -Filter "Name='Singularity.exe'" -ErrorAction Stop).Count){throw 'Singularity running: no asset writes'}
}
$manifestHash=Hash $manifestPath
$m=Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
if($m.schema -cne 'singularityvr-session-assets-delta-v1' -or @($m.files).Count -lt 1 -or
   -not $m.PSObject.Properties['protected'] -or @($m.protected).Count -lt 1){throw 'Unknown/empty session asset manifest'}
$seen=@{}
foreach($f in $m.files){
    $relative=[string]$f.path
    if(-not (Test-SessionAssetPath $relative)){
        throw "Target outside campaign delta: $relative"
    }
    if($seen.ContainsKey($relative.ToLowerInvariant())){throw 'Duplicate asset target'}
    $seen[$relative.ToLowerInvariant()]=$true
    if($f.after -cnotmatch '^[0-9a-f]{64}$' -or ($null -ne $f.before -and $f.before -cnotmatch '^[0-9a-f]{64}$')){throw 'Invalid SHA pin'}
    $source=Under (Join-Path $candidate 'payload') $relative
    if((Hash $source) -cne $f.after){throw "Candidate source changed: $relative"}
    [void](Under $game $relative)
    [void](Under $backup $relative)
}
foreach($p in $m.protected){
    if($p.path -cnotmatch '^[A-Za-z0-9_./ -]+$' -or $p.sha256 -cnotmatch '^[0-9a-f]{64}$' -or
       $seen.ContainsKey($p.path.ToLowerInvariant())){throw 'Invalid/overlapping protected target'}
    $seen[$p.path.ToLowerInvariant()]=$true
    [void](Under $game $p.path)
}
if($Action -eq 'ValidateCandidate'){Write-Output "Asset candidate: $(@($m.files).Count) verified files; not installed";return}
if(-not (Test-Path -LiteralPath (Join-Path $game 'Binaries\Singularity.exe') -PathType Leaf)){throw 'Not a Singularity installation'}
foreach($p in $m.protected){if((Hash (Under $game $p.path)) -cne $p.sha256){throw "Protected dependency drift: $($p.path); no writes"}}
if($Action -eq 'Preflight' -or $Action -eq 'Deploy'){
    Stopped
    foreach($f in $m.files){if((Hash (Under $game $f.path)) -cne $f.before){throw "Installed asset baseline drift: $($f.path)"}}
    if($Action -eq 'Preflight'){Write-Output 'Asset baseline preflight verified';return}
    if(Test-Path -LiteralPath $backup){throw 'Backup directory already exists; never overwrite'}
    New-Item -ItemType Directory -Path $backup | Out-Null
    foreach($f in $m.files){
        if($null -eq $f.before){continue}
        $dest=Under $backup $f.path
        New-Item -ItemType Directory -Path (Split-Path -Parent $dest) -Force | Out-Null
        Copy-Item -LiteralPath (Under $game $f.path) -Destination $dest
        if((Hash $dest) -cne $f.before){throw 'Backup verification failed'}
    }
    [ordered]@{schema='singularityvr-session-asset-backup-v1';manifestSha256=$manifestHash;gameRoot=$game;files=@($m.files)} |
        ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $backup 'receipt.json') -Encoding UTF8
}else{
    $receipt=Get-Content -Raw -LiteralPath (Join-Path $backup 'receipt.json') | ConvertFrom-Json
    if($receipt.schema -cne 'singularityvr-session-asset-backup-v1' -or $receipt.manifestSha256 -cne $manifestHash -or
       $receipt.gameRoot -ine $game){throw 'Asset backup/manifest identity mismatch'}
    # Check the COMPLETE transaction before the first restore write. A partial
    # deploy may have before OR after bytes; foreign edits are never clobbered.
    foreach($f in $m.files){
        if($null -ne $f.before -and (Hash (Under $backup $f.path)) -cne $f.before){throw 'Asset backup drift'}
        $live=Hash (Under $game $f.path)
        if($Action -eq 'ValidateLive'){
            if($live -cne $f.after){throw "Installed candidate drift: $($f.path)"}
        }elseif($live -cne $f.after -and $live -cne $f.before){throw "Foreign asset edit: $($f.path); no restore writes"}
    }
    if($Action -eq 'ValidateLive'){Write-Output 'All live assets and backups verified';return}
}
Stopped
foreach($f in $m.files){
    $target=Under $game $f.path
    $expected=if($Action -eq 'Deploy'){$f.after}else{$f.before}
    if((Hash $target) -ceq $expected){continue}
    Stopped
    $current=Hash $target
    if($Action -eq 'Deploy' -and $current -cne $f.before){throw 'Target drift during deploy'}
    if($Action -eq 'Restore' -and $current -cne $f.after){throw 'Target drift during restore'}
    if($null -eq $expected){Remove-Item -LiteralPath $target;continue}
    $source=if($Action -eq 'Deploy'){Under (Join-Path $candidate 'payload') $f.path}else{Under $backup $f.path}
    $temporary=$target+'.svr-session-stage'
    if(Test-Path -LiteralPath $temporary){throw 'Staging path already exists'}
    New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
    Copy-Item -LiteralPath $source -Destination $temporary
    if((Hash $temporary) -cne $expected){throw 'Staged asset SHA mismatch'}
    Stopped
    if((Hash $target) -cne $current){throw 'Asset changed during staging'}
    # Same-directory atomic replacement. Move-Item -Force can refuse an
    # existing Windows target (ERROR_ALREADY_EXISTS); never delete the baseline
    # first. The complete backup and drift checks above remain mandatory.
    if([IO.File]::Exists($target)){
        [IO.File]::Replace($temporary,$target,[System.Management.Automation.Language.NullString]::Value)
    }else{
        [IO.File]::Move($temporary,$target)
    }
    if((Hash $target) -cne $expected){throw 'Installed/restored asset SHA mismatch'}
}
foreach($f in $m.files){
    $expected=if($Action -eq 'Deploy'){$f.after}else{$f.before}
    if((Hash (Under $game $f.path)) -cne $expected){throw 'Final asset verification failed'}
}
Write-Output "Asset $Action complete: $(@($m.files).Count) exact file states verified"
