[CmdletBinding()]
param([Parameter(Mandatory)][string]$ManifestPath,[Parameter(Mandatory)][string]$OutputRoot,
 [string]$RealGameRoot='C:\Program Files (x86)\Steam\steamapps\common\Singularity')
# Private fake installation only. No real Steam writes, game launch or input.
Set-StrictMode -Version Latest;$ErrorActionPreference='Stop'
$repo=(Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$out=[IO.Path]::GetFullPath($OutputRoot)
if(-not $out.StartsWith($repo+'\deployment\hands-dubbing\',[StringComparison]::OrdinalIgnoreCase) -or (Test-Path $out)){throw 'Fresh owned private test directory required'}
$fake=Join-Path $out 'fake-Singularity';New-Item -ItemType Directory -Path $fake | Out-Null
$manifest=Get-Content $ManifestPath -Raw | ConvertFrom-Json
function Hash([string]$p){if(Test-Path -LiteralPath $p){return (Get-FileHash $p).Hash};return 'ABSENT'}
foreach($f in $manifest.Files){
 $dest=Join-Path $fake $f.RelativePath;New-Item -ItemType Directory -Path (Split-Path $dest) -Force | Out-Null
 if($f.BaselineSha256 -cne 'ABSENT'){
  Copy-Item -LiteralPath (Join-Path $RealGameRoot $f.RelativePath) -Destination $dest
  if((Hash $dest) -cne $f.BaselineSha256){throw 'Fixture baseline changed'}
 }
}
$tool=Join-Path $repo 'tools/test-prepared-assets.ps1'
& $tool -Action Install -ManifestPath $ManifestPath -GameRoot $fake -WhatIf
foreach($f in $manifest.Files){if((Hash (Join-Path $fake $f.RelativePath)) -cne $f.BaselineSha256){throw 'WhatIf wrote data'}}
$installed=@(& $tool -Action Install -ManifestPath $ManifestPath -GameRoot $fake)
$installed|Write-Output
$lines=@($installed|Where-Object {$_ -is [string] -and $_.StartsWith('ASSET BACKUP: ')})
if($lines.Count -ne 1){throw 'Backup identity missing'};$backup=$lines[0].Substring(14)
foreach($f in $manifest.Files){if((Hash (Join-Path $fake $f.RelativePath)) -cne $f.CandidateSha256){throw 'Install mismatch'}}
$saved=Join-Path $backup 'Binaries/texture_override.ini';$bytes=[IO.File]::ReadAllBytes($saved);$bad=$bytes.Clone();$bad[0]=$bad[0] -bxor 1;[IO.File]::WriteAllBytes($saved,$bad)
$refused=$false;try{& $tool -Action Restore -ManifestPath $ManifestPath -GameRoot $fake -BackupPath $backup}catch{$refused=$true}
if(-not $refused){throw 'Bad backup accepted'}
foreach($f in $manifest.Files){if((Hash (Join-Path $fake $f.RelativePath)) -cne $f.CandidateSha256){throw 'Partial restore before full backup check'}}
[IO.File]::WriteAllBytes($saved,$bytes)
& $tool -Action Restore -ManifestPath $ManifestPath -GameRoot $fake -BackupPath $backup
foreach($f in $manifest.Files){if((Hash (Join-Path $fake $f.RelativePath)) -cne $f.BaselineSha256){throw 'Restore mismatch'}}
$installed=@(& $tool -Action Install -ManifestPath $ManifestPath -GameRoot $fake)
$backup=(@($installed|Where-Object {$_ -is [string] -and $_.StartsWith('ASSET BACKUP: ')})[0]).Substring(14)
$userConfig=Join-Path $fake 'Binaries/texture_override.ini';[IO.File]::AppendAllText($userConfig,"`r`n; synthetic user edit preserved")
$userHash=Hash $userConfig;$refused=$false
try{& $tool -Action Restore -ManifestPath $ManifestPath -GameRoot $fake -BackupPath $backup}catch{$refused=$true}
if(-not $refused -or (Hash $userConfig) -cne $userHash){throw 'User conflict overwritten'}
foreach($f in $manifest.Files){if($f.RelativePath -cne 'Binaries\texture_override.ini' -and (Hash (Join-Path $fake $f.RelativePath)) -cne $f.BaselineSha256){throw 'Unconflicted file not restored'}}
# Restore only our synthetic fixture config, not the user's installation.
Copy-Item -LiteralPath (Join-Path $RealGameRoot 'Binaries/texture_override.ini') -Destination $userConfig
$badManifest=Join-Path $out 'bad.json';$manifest.Files[0].RelativePath='..\escape.xxx';$manifest|ConvertTo-Json -Depth 6|Set-Content $badManifest -Encoding UTF8
$refused=$false;try{& $tool -Action Install -ManifestPath $badManifest -GameRoot $fake}catch{$refused=$true}
if(-not $refused){throw 'Path escape accepted'}
Write-Output 'PASS: fake32 install/WhatIf/exact restore, full backup corruption refusal, conflict preservation, absent cleanup, path refusal'
