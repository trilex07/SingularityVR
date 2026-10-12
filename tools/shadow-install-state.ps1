[CmdletBinding()]
param(
    [ValidateSet('Snapshot','Restore')][string]$Action='Snapshot',
    [string]$BackupPath='',
    [string]$ExpectedDllSha256='9FBA308C91A2F246B336A54E35089A23CF676542CC0AFFB62B64E1B0599C4500',
    [string]$Provenance='5f05687f7823c9d0814edfe2ab320e7daef93802'
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$gameRoot='C:\Program Files (x86)\Steam\steamapps\common\Singularity'
$files=@((Join-Path $gameRoot 'Binaries\d3d9.dll'),(Join-Path $gameRoot 'Binaries\SingularityVR.ini'),(Join-Path $gameRoot 'RvGame\CookedPC\UI_SinglePlayer_SF.xxx'))
if (@(Get-Process -Name Singularity -ErrorAction SilentlyContinue).Count) { throw 'Never snapshot/deploy/restore a running game' }
if ($Action -eq 'Snapshot') {
    if ((Get-FileHash -LiteralPath $files[0]).Hash -ne $ExpectedDllSha256) { throw 'DLL differs from specified provenance' }
    if ((Get-FileHash -LiteralPath $files[1]).Hash -ne '61AEC56CF0F961C3D771DE186ADF469E39A978900DE8F8609086A965B4FE378B') { throw 'INI changed' }
    if ((Get-FileHash -LiteralPath $files[2]).Hash -ne '5B2EA469A3EE06FF154AC78BD58604E8034970D4837F289E5C22503C6A673CDE') { throw 'Accepted RTT package changed' }
    $root=Join-Path (Split-Path $PSScriptRoot -Parent) 'deployment\shadow-backups'
    $backup=Join-Path $root (Get-Date -Format 'yyyyMMdd-HHmmss-fff')
    if (Test-Path -LiteralPath $backup) { throw 'Backup collision; never overwrite an earlier manifest' }
    New-Item -ItemType Directory -Path $backup | Out-Null
    $entries=@(foreach($target in $files) {
        $hash=(Get-FileHash -LiteralPath $target).Hash
        $saved=Join-Path $backup ([IO.Path]::GetFileName($target))
        Copy-Item -LiteralPath $target -Destination $saved
        if ((Get-FileHash -LiteralPath $saved).Hash -ne $hash) { throw 'Backup hash mismatch' }
        [ordered]@{target=$target;saved=$saved;sha256=$hash}
    })
    [ordered]@{createdAt=(Get-Date).ToString('o');provenance=$Provenance;files=$entries} |
        ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $backup 'manifest.json') -Encoding utf8
    Write-Output "VERIFIED SNAPSHOT: $backup"
} else {
    if (-not $BackupPath) { throw 'Restore requires exact BackupPath' }
    $manifest=Get-Content -LiteralPath (Join-Path $BackupPath 'manifest.json') -Raw | ConvertFrom-Json
    if (@($manifest.files).Count -ne 3) { throw 'Incomplete restore manifest' }
    foreach($entry in $manifest.files) {
        if ($entry.target -notin $files) { throw 'Unexpected restore destination' }
        if ((Get-FileHash -LiteralPath $entry.saved).Hash -ne $entry.sha256) { throw 'Corrupt backup; no writes performed' }
    }
    foreach($entry in $manifest.files) {
        if (@(Get-Process -Name Singularity -ErrorAction SilentlyContinue).Count) { throw 'Game started; abort restore' }
        $stage=$entry.target+'.shadow-restore.new'
        Copy-Item -LiteralPath $entry.saved -Destination $stage -Force
        if ((Get-FileHash -LiteralPath $stage).Hash -ne $entry.sha256) { throw 'Staging hash mismatch' }
        Move-Item -LiteralPath $stage -Destination $entry.target -Force
        if ((Get-FileHash -LiteralPath $entry.target).Hash -ne $entry.sha256) { throw 'Restored hash mismatch' }
        Write-Output ('VERIFIED RESTORE: '+$entry.target+' '+$entry.sha256)
    }
}
