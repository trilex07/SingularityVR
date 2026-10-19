[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidateSet('ValidateCandidate','Deploy','ValidateLive','Restore')][string]$Action,
    [Parameter(Mandatory)][string]$CandidateDir,
    [Parameter(Mandatory)][string]$BackupDir,
    [string]$SteamRoot='C:\Program Files (x86)\Steam'
)
# MAIN only. A narrow lease for ONE unpinned package, not permission to change
# accepted-profile files. Exact original/candidate pair independently decoded.
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$allowed=[IO.Path]::GetFullPath((Join-Path $repo 'build\next-weapon-reload')).TrimEnd('\')+'\'
$candidate=[IO.Path]::GetFullPath($CandidateDir)
$backup=[IO.Path]::GetFullPath($BackupDir)
foreach($path in @($candidate,$backup)){
    if(-not $path.StartsWith($allowed,[StringComparison]::OrdinalIgnoreCase)){throw 'NPC lease paths must be inside this checkout build/next-weapon-reload'}
}
$before='4153C4889C90635069ED226D918CDB76BE618F46B1BAEA152AC10CFFDF59E9EF'
$after='FF38C0AA32A54E1D3689EB5676DF4E614E162C180850B478CC14FDE997D2FBE6'
function Hash([string]$path){return (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToUpperInvariant()}
function AssertStopped {
    if(-not (Get-CimInstance Win32_Process -Filter "ProcessId=$PID" -ErrorAction Stop)){throw 'Full process visibility required'}
    if(@(Get-CimInstance Win32_Process -Filter "Name='Singularity.exe'" -ErrorAction Stop).Count){throw 'Game running: no package writes'}
}
$source=Join-Path $candidate 'RvGame.xxx'
$report=Get-Content -Raw -LiteralPath (Join-Path $candidate 'report.json') | ConvertFrom-Json
if((Hash $source) -cne $after -or $report.input_sha256.ToUpperInvariant() -cne $before -or
   $report.candidate_sha256.ToUpperInvariant() -cne $after -or -not $report.native_decoder_roundtrip -or
   $report.package_offset -ne 6076755 -or $report.old_twips -ne 340 -or $report.new_twips -ne 204){throw 'NPC candidate/report not the reviewed exact pair'}
if($Action -eq 'ValidateCandidate'){Write-Output "NPC candidate verified: $after (not installed)";return}
$gameRoot=Join-Path ([IO.Path]::GetFullPath($SteamRoot)) 'steamapps\common\Singularity'
if(-not (Test-Path -LiteralPath (Join-Path $gameRoot 'Binaries\Singularity.exe') -PathType Leaf)){throw 'Not the Singularity installation'}
$target=Join-Path $gameRoot 'RvGame\CookedPC\RvGame.xxx'
$saved=Join-Path $backup 'RvGame.xxx'
if($Action -eq 'Deploy'){
    AssertStopped
    if((Hash $target) -cne $before -or (Test-Path -LiteralPath $backup)){throw 'Original package drift or existing backup; refusing deploy'}
    New-Item -ItemType Directory -Path $backup | Out-Null
    Copy-Item -LiteralPath $target -Destination $saved
    if((Hash $saved) -cne $before){throw 'NPC original backup SHA mismatch'}
}else{
    if((Hash $saved) -cne $before -or (Hash $target) -cne $after){throw 'NPC live/backup drift; refusing overwrite'}
    if($Action -eq 'ValidateLive'){Write-Output 'NPC installed candidate and original backup verified';return}
}
AssertStopped
$temporary=$target+'.svr-npc-test-tmp'
if(Test-Path -LiteralPath $temporary){throw 'Existing NPC temporary file; refusing overwrite'}
$from=if($Action -eq 'Deploy'){$source}else{$saved}
$expected=if($Action -eq 'Deploy'){$after}else{$before}
Copy-Item -LiteralPath $from -Destination $temporary
if((Hash $temporary) -cne $expected){throw 'NPC staged SHA mismatch'}
AssertStopped
if((Hash $target) -cne $(if($Action -eq 'Deploy'){$before}else{$after})){throw 'NPC target changed during staging'}
Move-Item -LiteralPath $temporary -Destination $target -Force
if((Hash $target) -cne $expected){throw 'NPC installed/restored SHA mismatch'}
Write-Output "NPC $Action verified: $expected"
