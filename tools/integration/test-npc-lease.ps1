[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$OriginalPackage,
    [Parameter(Mandatory)][string]$CandidateDir
)
# Read a reviewed original; all writes are to a fresh private fake install.
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$base=Join-Path $repo ('build\next-weapon-reload\npc-lease-fixture-'+[guid]::NewGuid().ToString('N'))
$steam=Join-Path $base 'steam'
$game=Join-Path $steam 'steamapps\common\Singularity'
$target=Join-Path $game 'RvGame\CookedPC\RvGame.xxx'
$backup=Join-Path $base 'backup'
$adapter=Join-Path $repo 'tools\title\deploy_npc_name_test.ps1'
$before='4153C4889C90635069ED226D918CDB76BE618F46B1BAEA152AC10CFFDF59E9EF'
if((Get-FileHash -LiteralPath $OriginalPackage).Hash -cne $before){throw 'Not the reviewed original package'}
$global:npcLeaseFixtureGameRunning=$false
function Get-CimInstance {
    param($ClassName,$Filter,$ErrorAction)
    if($Filter -like 'ProcessId=*'){return [pscustomobject]@{ProcessId=$PID}}
    if($global:npcLeaseFixtureGameRunning){return [pscustomobject]@{Name='Singularity.exe';ProcessId=12345}}
    return @()
}
function MustRefuse([scriptblock]$call){$refused=$false;try{& $call | Out-Null}catch{$refused=$true};if(-not $refused){throw 'Expected guarded refusal'}}
New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $game 'Binaries') -Force | Out-Null
[IO.File]::WriteAllText((Join-Path $game 'Binaries\Singularity.exe'),'FAKE FIXTURE; NEVER EXECUTED')
Copy-Item -LiteralPath $OriginalPackage -Destination $target
$global:npcLeaseFixtureGameRunning=$true
MustRefuse {& $adapter -Action Deploy -CandidateDir $CandidateDir -BackupDir $backup -SteamRoot $steam}
if(Test-Path -LiteralPath $backup){throw 'Running-game refusal happened after writes'}
$global:npcLeaseFixtureGameRunning=$false
& $adapter -Action Deploy -CandidateDir $CandidateDir -BackupDir $backup -SteamRoot $steam
& $adapter -Action ValidateLive -CandidateDir $CandidateDir -BackupDir $backup -SteamRoot $steam
[IO.File]::WriteAllText($target,'foreign edit')
MustRefuse {& $adapter -Action Restore -CandidateDir $CandidateDir -BackupDir $backup -SteamRoot $steam}
if([IO.File]::ReadAllText($target) -cne 'foreign edit'){throw 'Foreign package was overwritten'}
Copy-Item -LiteralPath (Join-Path $CandidateDir 'RvGame.xxx') -Destination $target -Force
$saved=Join-Path $backup 'RvGame.xxx'
[IO.File]::WriteAllText($saved,'bad backup')
MustRefuse {& $adapter -Action Restore -CandidateDir $CandidateDir -BackupDir $backup -SteamRoot $steam}
Copy-Item -LiteralPath $OriginalPackage -Destination $saved -Force
& $adapter -Action Restore -CandidateDir $CandidateDir -BackupDir $backup -SteamRoot $steam
if((Get-FileHash -LiteralPath $target).Hash -cne $before){throw 'NPC exact restore failed'}
Write-Output 'PASS: NPC fake-install exact pair, running-game refusal, drift preservation, corrupt backup refusal and exact restore'
Remove-Variable -Name npcLeaseFixtureGameRunning -Scope Global
