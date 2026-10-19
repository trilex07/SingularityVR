# Compose disjoint, explicitly approved test deltas; never alter the accepted profile.
function New-CombinedSessionProfileTicket {
    param([string]$GameRoot,[string]$ProfilePath,[string]$ChangePlanPath,
          [string]$ManifestPath,[string]$OutputPath,[object[]]$FileMap)
    $profile=Read-GuardJson $ProfilePath
    $corePlan=Read-GuardJson $ChangePlanPath
    $delta=Read-GuardJson $ManifestPath
    if($delta.schema -cne 'singularityvr-session-assets-delta-v1'){throw 'Unknown combined asset schema'}
    if(Test-Path -LiteralPath $OutputPath){throw 'Never overwrite a composed profile plan'}
    $pins=@{}
    foreach($pin in $profile.files){$pins[$pin.path]=$pin.sha256}
    $allowed=@($corePlan.files)
    $map=@($FileMap)
    $seen=@{}
    foreach($row in $delta.files){
        $relative=ConvertTo-GuardRelative $row.path
        if($seen.ContainsKey($relative)){throw 'Duplicate combined asset target'}
        $seen[$relative]=$true
        if(-not $pins.ContainsKey($relative)){continue}
        # Only these two protected manifests are owned by the reviewed asset delta.
        if($relative -cnotin @('Binaries/vr-assets/localization/pl/polish_text.jsonl',
                              'Binaries/vr-assets/localization/pl/polish_story_voice.jsonl')){
            throw "Asset delta overlaps a frozen profile dependency: $relative"
        }
        Assert-GuardHash $row.before
        Assert-GuardHash $row.after
        if(-not $row.before -or -not $row.after -or $row.before -ine $pins[$relative] -or
           @($allowed | Where-Object path -ieq $relative).Count){throw 'Conflicting combined profile allowance'}
        $source=Resolve-GuardPath (Join-Path (Split-Path -Parent $ManifestPath) 'payload') $relative
        if((Get-GuardHash $source) -ine $row.after){throw 'Combined source SHA drift'}
        $allowed += [pscustomobject]@{path=$relative;before=$row.before;after=$row.after;
            reason='User-requested latest Polish campaign text/story manifest; exact sealed delta, restored after exit'}
        $map += [pscustomobject]@{Target=$relative;Source=$source}
    }
    $corePlan.files=$allowed
    $corePlan.reason+='; exact sealed campaign manifest allowances for combined postflight only'
    $corePlan | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $OutputPath -Encoding UTF8
    return Assert-AcceptedDeployment -GameRoot $GameRoot -ProfilePath $ProfilePath `
        -ChangePlanPath $OutputPath -FileMap $map -Direction Install -Phase Post
}
