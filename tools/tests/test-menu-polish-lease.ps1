[CmdletBinding()]
param([Parameter(Mandatory)][string]$ProfilePath,[Parameter(Mandatory)][string]$CandidateDirectory,
 [Parameter(Mandatory)][string]$OutputRoot)
# Read real baseline files, write only a fresh private fake Steam installation.
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$repo=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$out=[IO.Path]::GetFullPath($OutputRoot)
if(-not $out.StartsWith($repo+'\build\',[StringComparison]::OrdinalIgnoreCase) -or (Test-Path $out)){throw 'Fresh owned build fixture required'}
$fake=Join-Path $out 'steamapps\common\Singularity'
$real='C:\Program Files (x86)\Steam\steamapps\common\Singularity'
$profile=Get-Content $ProfilePath -Raw | ConvertFrom-Json
foreach($relative in @($profile.files|ForEach-Object path)+@($profile.iniPath)){
 $source=Join-Path $real $relative
 if(Test-Path -LiteralPath $source -PathType Leaf){
  $target=Join-Path $fake $relative
  New-Item -ItemType Directory -Path (Split-Path $target) -Force | Out-Null
  Copy-Item -LiteralPath $source -Destination $target
 }
}
$installed=@(& (Join-Path $repo 'tools/deploy-singularityvr.ps1') -Action Install -DllOnly -IncludeIni -ArtifactDir $CandidateDirectory -SteamRoot $out -AcceptedProfilePath $ProfilePath -ChangePlanPath (Join-Path $CandidateDirectory 'change-plan.json') 6>&1 | ForEach-Object {[string]$_})
$installed|Write-Output
$backupLines=@($installed|Where-Object {$_ -is [string] -and $_.StartsWith('Backup: ')})
if($backupLines.Count -ne 1){throw 'Missing fixture backup'}
$backup=$backupLines[0].Substring(8)
& (Join-Path $repo 'tools/watch-menu-polish-restore.ps1') -BackupPath $backup -AcceptedProfilePath $ProfilePath -ChangePlanPath (Join-Path $CandidateDirectory 'change-plan.json') -SteamRoot $out -LaunchWindowSeconds 60
if(-not $?){throw 'Fake no-launch lease failed'}
foreach($name in @('d3d9.dll','SingularityVR.ini')){
 if((Get-FileHash (Join-Path $fake ('Binaries/'+$name))).Hash -cne (Get-FileHash (Join-Path $real ('Binaries/'+$name))).Hash){throw 'Fake fixture restore mismatch'}
}
Write-Output 'PASS: fake DLL/INI deployment and timed guarded rollback; real installation read only'
