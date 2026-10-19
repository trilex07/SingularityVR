Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'test-accepted-profile-guard.ps1')
. (Join-Path $repo 'tools/integration/CombinedSessionProfile.ps1')
$pHash=Get-GuardHash $profilePath
$cHash=Get-GuardHash $planPath
$deltaRoot=Join-Path $testRoot 'campaign'
$rows=@()
$old=@{}
foreach($name in @('polish_text.jsonl','polish_story_voice.jsonl')){
    $relative='Binaries/vr-assets/localization/pl/'+$name
    $live=Resolve-GuardPath $game $relative
    $old[$relative]=[IO.File]::ReadAllText($live)
    $source=Join-Path $deltaRoot ('payload/'+$relative)
    Write-Fixture $source ('new complete translation '+$name)
    $rows += [pscustomobject]@{path=$relative;before=(Get-GuardHash $live);after=(Get-GuardHash $source)}
    [IO.File]::Copy($source,$live,$true)
}
[IO.File]::Copy($candidateDll,$dllPath,$true)
$manifest=Join-Path $deltaRoot 'manifest.json'
Write-JsonFixture $manifest ([pscustomobject]@{schema='singularityvr-session-assets-delta-v1';files=$rows})
$runtimeMap=@([pscustomobject]@{Target='Binaries/d3d9.dll';Source=$candidateDll})
Must-Reject {Assert-AcceptedDeployment -GameRoot $game -ProfilePath $profilePath -ChangePlanPath $planPath -FileMap $runtimeMap -Phase Post} 'reproduce combined protected-manifest conflict'
function Combined([string]$label){
    New-CombinedSessionProfileTicket -GameRoot $game -ProfilePath $profilePath -ChangePlanPath $planPath `
        -ManifestPath $manifest -OutputPath (Join-Path $testRoot ($label+'.json')) -FileMap $runtimeMap
}
$composed=Combined 'composed'
Assert-True ($composed.Sources.Count -eq 3) 'exact DLL plus two manifest write identities required'
Assert-True ((Get-GuardHash $profilePath) -ceq $pHash -and (Get-GuardHash $planPath) -ceq $cHash) 'composition changed frozen profile/core plan'
Must-Reject {Combined 'composed'} 'composed allowance overwrite'
$frozen=Resolve-GuardPath $game 'Binaries/vr-assets/knife/knife.svrk'
$frozenBefore=[IO.File]::ReadAllText($frozen)
Write-Fixture $frozen 'foreign knife'
Must-Reject {Combined 'foreign'} 'composition must still protect knife'
Write-Fixture $frozen $frozenBefore
$before=$rows[0].before;$rows[0].before='0'*64
Write-JsonFixture $manifest ([pscustomobject]@{schema='singularityvr-session-assets-delta-v1';files=$rows})
Must-Reject {Combined 'bad-before'} 'baseline cannot be blessed from live after bytes'
$rows[0].before=$before
Write-JsonFixture $manifest ([pscustomobject]@{schema='singularityvr-session-assets-delta-v1';files=@($rows)+@([pscustomobject]@{path='Binaries/vr-assets/knife/knife.svrk';before=(Get-GuardHash $frozen);after=('1'*64)})})
Must-Reject {Combined 'knife-allowance'} 'reject frozen dependency overlap'
foreach($row in $rows){Write-Fixture (Resolve-GuardPath $game $row.path) $old[$row.path]}
[IO.File]::Copy($restoreDll,$dllPath,$true)
[void](Assert-AcceptedDeployment -GameRoot $game -ProfilePath $profilePath)
Write-Output 'PASS combined manifest postflight; immutable accepted profile/core plan, reject drift/bad pins/broad allowances, exact baseline restore; ZERO Steam'
