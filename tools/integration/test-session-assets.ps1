$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$base=Join-Path $repo ('build\next-weapon-reload\asset-lease-fixture-'+[guid]::NewGuid().ToString('N'))
$game=Join-Path $base 'steamapps\common\Singularity'
$candidate=Join-Path $base 'candidate'
$backup=Join-Path $base 'backup'
$manifest=Join-Path $candidate 'asset-manifest.json'
$adapter=Join-Path $PSScriptRoot 'session-assets.ps1'
# A fake process provider is scoped ONLY to this deterministic fixture. All
# paths are private workspace children and nothing targets the real install.
$global:sessionAssetFixtureGameRunning=$false
function Get-CimInstance {
    param($ClassName,$Filter,$ErrorAction)
    if($Filter -like 'ProcessId=*'){return [pscustomobject]@{ProcessId=$PID}}
    if($global:sessionAssetFixtureGameRunning){return [pscustomobject]@{Name='Singularity.exe';ProcessId=12345}}
    return @()
}
function Put([string]$p,[string]$s){
    New-Item -ItemType Directory -Path (Split-Path -Parent $p) -Force | Out-Null
    [IO.File]::WriteAllText($p,$s,[Text.UTF8Encoding]::new($false))
}
function Hash([string]$p){if(Test-Path -LiteralPath $p){return (Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()};return $null}
function Assert([bool]$condition,[string]$message){if(-not $condition){throw $message}}
function MustRefuse([scriptblock]$call){$refused=$false;try{& $call | Out-Null}catch{$refused=$true};Assert $refused 'Expected guarded refusal'}
Put (Join-Path $game 'Binaries\Singularity.exe') 'FAKE NON-EXECUTABLE FIXTURE; NEVER LAUNCHED'
$files=@()
foreach($relative in @('Binaries/texture-override-remaster/revert_d2.dds','Binaries/vr-assets/localization/pl/polish_text.jsonl','Binaries/vr-assets/audio/polish/story/new.wav')){
    $live=Join-Path $game $relative
    if(-not $relative.EndsWith('new.wav')){Put $live ('before '+$relative)}
    $source=Join-Path (Join-Path $candidate 'payload') $relative
    Put $source ('candidate '+$relative)
    $files += [pscustomobject]@{path=$relative;before=(Hash $live);after=(Hash $source)}
}
$protected=Join-Path $game 'Binaries\dinput8.dll'
Put $protected 'protected injector'
[ordered]@{schema='singularityvr-session-assets-delta-v1';files=$files;protected=@(@{path='Binaries/dinput8.dll';sha256=(Hash $protected)})} | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $manifest -Encoding UTF8
& $adapter -Action ValidateCandidate -ManifestPath $manifest -BackupDir $backup -GameRoot $game
$global:sessionAssetFixtureGameRunning=$true
MustRefuse {& $adapter -Action Deploy -ManifestPath $manifest -BackupDir $backup -GameRoot $game}
Assert (-not (Test-Path -LiteralPath $backup)) 'Running game must refuse before backup/write'
$global:sessionAssetFixtureGameRunning=$false
& $adapter -Action Deploy -ManifestPath $manifest -BackupDir $backup -GameRoot $game
& $adapter -Action ValidateLive -ManifestPath $manifest -BackupDir $backup -GameRoot $game
Put $protected 'foreign injector'
MustRefuse {& $adapter -Action Restore -ManifestPath $manifest -BackupDir $backup -GameRoot $game}
Assert ((Hash (Join-Path $game $files[0].path)) -ceq $files[0].after) 'Protected drift must refuse before any writes'
Put $protected 'protected injector'
$foreign=Join-Path $game $files[0].path
Put $foreign 'foreign change'
MustRefuse {& $adapter -Action Restore -ManifestPath $manifest -BackupDir $backup -GameRoot $game}
Assert ((Hash (Join-Path $game $files[1].path)) -ceq $files[1].after) 'Foreign drift must refuse before ANY restore writes'
Copy-Item -LiteralPath (Join-Path (Join-Path $candidate 'payload') $files[0].path) -Destination $foreign -Force
$saved=Join-Path $backup $files[0].path
Put $saved 'corrupt backup'
MustRefuse {& $adapter -Action Restore -ManifestPath $manifest -BackupDir $backup -GameRoot $game}
Put $saved ('before '+$files[0].path)
& $adapter -Action Restore -ManifestPath $manifest -BackupDir $backup -GameRoot $game
foreach($f in $files){Assert ((Hash (Join-Path $game $f.path)) -ceq $f.before) 'Exact old state not restored'}
Write-Output 'PASS: fake install/deploy/full verification, running-game refusal, foreign drift preservation, bad backup refusal, exact restore and absent-file restoration'
Remove-Variable -Name sessionAssetFixtureGameRunning -Scope Global
