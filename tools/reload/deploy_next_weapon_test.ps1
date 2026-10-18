[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidateSet('Deploy','Restore')][string]$Action,
    [Parameter(Mandatory)][string]$GameBin,
    [Parameter(Mandatory)][string]$BackupDir,
    [string]$CandidateRoot
)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$bin=[IO.Path]::GetFullPath($GameBin)
$backup=[IO.Path]::GetFullPath($BackupDir)
if(-not $backup.StartsWith((Join-Path $repo 'build').TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase)){
    throw 'BackupDir must be inside this checkout/build'
}
if((Split-Path $bin -Leaf) -ine 'Binaries' -or
   -not (Test-Path -LiteralPath (Join-Path $bin 'Singularity.exe') -PathType Leaf) -or
   -not (Test-Path -LiteralPath (Join-Path $bin 'd3d9.dll') -PathType Leaf)){
    throw 'GameBin is not the verified Singularity Binaries folder'
}
if(Get-Process Singularity -ErrorAction SilentlyContinue){throw 'Singularity.exe is running; no files changed'}
$relative=@()
foreach($weapon in @('valkyrie','volk','kasimov')){
    foreach($suffix in @('clip.svrk','clip.svrt','reload.svwa')){
        $relative+= "vr-assets/reload/$weapon/${weapon}_$suffix"
    }
}
function Hash([string]$path){
    if(-not (Test-Path -LiteralPath $path -PathType Leaf)){return $null}
    return (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToUpperInvariant()
}
function Target([string]$rel){
    if($rel -cnotin $relative){throw "Unlisted asset: $rel"}
    $full=[IO.Path]::GetFullPath((Join-Path $bin $rel))
    if(-not $full.StartsWith($bin.TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase)){
        throw 'Target escaped Binaries'
    }
    return $full
}
if($Action -eq 'Deploy'){
    if(-not $CandidateRoot){throw 'Deploy requires CandidateRoot'}
    if(Test-Path -LiteralPath $backup){throw 'Refusing to overwrite asset backup'}
    $candidate=[IO.Path]::GetFullPath($CandidateRoot)
    $package=Get-Content -Raw -LiteralPath (Join-Path $candidate 'manifest.json') | ConvertFrom-Json
    $receipt=@()
    foreach($rel in $relative){
        $source=Join-Path $candidate $rel
        $entry=@($package.files | Where-Object path -ceq $rel)
        if($entry.Count -ne 1 -or -not (Test-Path -LiteralPath $source -PathType Leaf) -or
           (Hash $source) -cne $entry[0].sha256.ToUpperInvariant()){
            throw "Candidate asset hash/manifest mismatch: $rel"
        }
        $target=Target $rel
        $receipt += [pscustomobject]@{path=$rel;before=(Hash $target);after=(Hash $source)}
    }
    New-Item -ItemType Directory -Path $backup | Out-Null
    foreach($entry in $receipt){
        if($entry.before){
            $saved=Join-Path $backup $entry.path
            New-Item -ItemType Directory -Force -Path (Split-Path $saved) | Out-Null
            Copy-Item -LiteralPath (Target $entry.path) -Destination $saved
            if((Hash $saved) -cne $entry.before){throw "Backup hash mismatch: $($entry.path)"}
        }
    }
    $receipt | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $backup 'assets.json') -Encoding UTF8
    foreach($entry in $receipt){
        $target=Target $entry.path
        New-Item -ItemType Directory -Force -Path (Split-Path $target) | Out-Null
        $temp=$target+'.svrtest-tmp'
        if(Test-Path -LiteralPath $temp){throw "Refusing existing temporary file: $temp"}
        Copy-Item -LiteralPath (Join-Path $candidate $entry.path) -Destination $temp
        if((Hash $temp) -cne $entry.after){throw "Staged hash mismatch: $($entry.path)"}
        Move-Item -LiteralPath $temp -Destination $target -Force
        if((Hash $target) -cne $entry.after){throw "Installed hash mismatch: $($entry.path)"}
    }
    Write-Output "Next-weapon assets deployed and verified: $backup"
}else{
    $manifest=Join-Path $backup 'assets.json'
    $receipt=Get-Content -Raw -LiteralPath $manifest | ConvertFrom-Json
    if($receipt.Count -ne 9){throw 'Asset rollback manifest must have nine exact files'}
    foreach($entry in $receipt){
        $target=Target $entry.path
        if((Hash $target) -cne $entry.after){throw "Installed asset drift; refusing rollback: $($entry.path)"}
        if($entry.before -and (Hash (Join-Path $backup $entry.path)) -cne $entry.before){
            throw "Backup drift; refusing rollback: $($entry.path)"
        }
    }
    foreach($entry in $receipt){
        $target=Target $entry.path
        if($entry.before){Copy-Item -LiteralPath (Join-Path $backup $entry.path) -Destination $target -Force}
        else{Remove-Item -LiteralPath $target -Force}
        if((Hash $target) -cne $entry.before){throw "Restored asset hash mismatch: $($entry.path)"}
    }
    Write-Output 'Next-weapon asset rollback verified; no directories removed'
}
