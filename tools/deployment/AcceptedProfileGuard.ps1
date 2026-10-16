# Read-only functions. Dot-sourcing performs no I/O or game/process operations.
Set-StrictMode -Version Latest

function Get-GuardProperty($Object,[string]$Name) {
    $p=$Object.PSObject.Properties[$Name]
    if($null -eq $p){throw "Missing required guard field: $Name"}
    return $p.Value
}
function Read-GuardJson([string]$Path) {
    $item=Get-Item -LiteralPath $Path -ErrorAction Stop
    if($item.Length -gt 8MB){throw 'Guard JSON exceeds 8 MiB'}
    return (Get-Content -LiteralPath $Path -Raw -Encoding UTF8 | ConvertFrom-Json)
}
function Get-GuardHash([string]$Path) {
    if((Test-Path -LiteralPath $Path) -and -not (Test-Path -LiteralPath $Path -PathType Leaf)){
        throw "Expected file, found directory: $Path"
    }
    if(-not (Test-Path -LiteralPath $Path -PathType Leaf)){return $null}
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToUpperInvariant()
}
function ConvertTo-GuardRelative([string]$Path) {
    $p=$Path.Replace('\','/')
    if(-not $p -or [IO.Path]::IsPathRooted($p) -or $p -match '[\x00-\x1f:*?"<>|\[\]]'){
        throw "Unsafe guard relative path: $Path"
    }
    foreach($segment in $p.Split('/')){
        if(-not $segment -or $segment -in @('.','..') -or $segment -match '[ .]$' -or
           $segment -match '^(?i:CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\.|$)'){
            throw "Unsafe guard path segment: $Path"
        }
    }
    return $p
}
function Resolve-GuardPath([string]$Root,[string]$Relative) {
    $rootFull=[IO.Path]::GetFullPath($Root).TrimEnd('\','/')
    $relativeSafe=ConvertTo-GuardRelative $Relative
    $full=[IO.Path]::GetFullPath((Join-Path $rootFull $relativeSafe))
    if(-not $full.StartsWith($rootFull+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){
        throw 'Guard path escaped root'
    }
    # No junction/symlink escape, including the root or a not-yet-created leaf.
    $walk=$full
    while($walk.Length -ge $rootFull.Length){
        if(Test-Path -LiteralPath $walk){
            if((Get-Item -LiteralPath $walk -Force).Attributes -band [IO.FileAttributes]::ReparsePoint){
                throw "Guard refuses reparse point: $walk"
            }
        }
        if($walk -eq $rootFull){break}
        $walk=Split-Path -Parent $walk
    }
    return $full
}
function Assert-GuardHash($Value) {
    if($null -ne $Value -and ([string]$Value -notmatch '^[0-9A-Fa-f]{64}$')){
        throw 'Expected exact SHA-256 or null (known absent file)'
    }
}
function Read-GuardIni([string]$Path) {
    $item=Get-Item -LiteralPath $Path -ErrorAction Stop
    if($item.Length -gt 2MB){throw 'INI exceeds 2 MiB'}
    $text=([Text.UTF8Encoding]::new($false,$true)).GetString([IO.File]::ReadAllBytes($Path)).TrimStart([char]0xFEFF)
    $result=@{};$section=''
    foreach($line in ($text -split '\r?\n')){
        if($line -match '^\s*(?:[;#].*)?$'){continue}
        if($line -match '^\s*\[([^\]/]+)\]\s*$'){$section=$Matches[1].Trim();continue}
        if($section -and $line -match '^\s*([^=/]+?)\s*=\s*(.*?)\s*$'){
            $key=$section+'/'+$Matches[1].Trim()
            if($result.ContainsKey($key)){throw "Duplicate INI key: $key"}
            $result[$key]=$Matches[2];continue
        }
        throw "Ambiguous INI line: $line"
    }
    return $result
}
function Test-GuardSame($A,$B) {
    if($null -eq $A -or $null -eq $B){return ($null -eq $A -and $null -eq $B)}
    return [string]::Equals([string]$A,[string]$B,[StringComparison]::Ordinal)
}
function Assert-AcceptedDeployment {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$GameRoot,
        [Parameter(Mandatory)][string]$ProfilePath,
        [string]$ChangePlanPath='',
        [object[]]$FileMap=@(),
        [ValidateSet('Install','Rollback')][string]$Direction='Install',
        [ValidateSet('Pre','Post')][string]$Phase='Pre'
    )
    $profile=Read-GuardJson $ProfilePath
    if((Get-GuardProperty $profile 'schema') -ne 'singularityvr-accepted-profile-v1' -or
       (Get-GuardProperty $profile 'approved') -isnot [bool] -or -not $profile.approved -or
       -not (Get-GuardProperty $profile 'acceptedBy') -or -not (Get-GuardProperty $profile 'acceptanceScope')){
        throw 'A reviewed, approved accepted profile is required; never infer it from live files'
    }
    $profileHash=Get-GuardHash $ProfilePath
    $iniRelative=ConvertTo-GuardRelative (Get-GuardProperty $profile 'iniPath')
    $iniPath=Resolve-GuardPath $GameRoot $iniRelative
    $baseIni=@{}
    foreach($p in (Get-GuardProperty $profile 'ini').PSObject.Properties){
        if($p.Name -notmatch '^[^/]+/[^/]+$' -or $null -eq $p.Value -or $p.Value -isnot [string]){throw 'Invalid profile INI entry'}
        $baseIni[$p.Name]=$p.Value
    }
    foreach($key in @('Localization/PolishLanguage','Render/GloveNativeEnabled','Render/GloveRendererEnabled',
        'Input/HipKnifeEnabled','Render/KnifeHandGripEnabled','Render/HandThumbPoseFix','Render/KnifeSceneLighting')){
        if(-not $baseIni.ContainsKey($key)){throw "Profile omits protected feature key: $key"}
    }
    $ignore=@(Get-GuardProperty $profile 'ignoredIniKeys')
    foreach($key in $ignore){
        if($key -notin @('Render/AutoResX','Render/AutoResY')){throw "INI ignore not permitted: $key"}
    }
    $baseFiles=@{}
    foreach($entry in @(Get-GuardProperty $profile 'files')){
        $path=ConvertTo-GuardRelative (Get-GuardProperty $entry 'path')
        $hash=Get-GuardProperty $entry 'sha256';Assert-GuardHash $hash
        if($null -ne $hash){$hash=$hash.ToUpperInvariant()}
        if($path -ieq $iniRelative -or $baseFiles.ContainsKey($path)){throw "Duplicate/INI file pin: $path"}
        [void](Resolve-GuardPath $GameRoot $path);$baseFiles[$path]=$hash
    }
    if(-not $baseFiles.ContainsKey('Binaries/d3d9.dll') -or $null -eq $baseFiles['Binaries/d3d9.dll']){
        throw 'Profile must pin its accepted renderer DLL'
    }
    $dependencies=@{
        'Render/NativeGloveCuffClosure'=@('Binaries/vr-assets/hud/katorga/glove-animation/native_cuffs.svnc')
        'Render/GloveNativeEnabled'=@('Binaries/vr-assets/hud/katorga/glove-animation/native_hands.svni')
        'Render/HandThumbPoseFix'=@('Binaries/vr-assets/hud/katorga/glove-animation/hand_thumb.svnt')
        'Input/HipKnifeEnabled'=@('Binaries/vr-assets/knife/knife.svrk','Binaries/vr-assets/knife/knife.svrt')
        'Render/KnifeHandGripEnabled'=@('Binaries/vr-assets/knife/knife_hand.svng')
        'Render/KnifeSceneLighting'=@('Binaries/vr-assets/knife/knife.svnl','Binaries/vr-assets/knife/knife_normal.svrt','Binaries/vr-assets/knife/knife_specular.svrt')
        'Localization/PolishLanguage'=@('Binaries/vr-assets/localization/pl/polish_text.jsonl','Binaries/vr-assets/localization/pl/polish_voice.jsonl','Binaries/vr-assets/localization/pl/polish_story_voice.jsonl')
    }
    foreach($key in $dependencies.Keys){if($baseIni[$key] -eq '1'){
        foreach($path in $dependencies[$key]){if(-not $baseFiles.ContainsKey($path) -or $null -eq $baseFiles[$path]){
            throw "Enabled feature dependency is not pinned: $path"
        }}
    }}
    $planFiles=@{};$planIni=@{}
    if($ChangePlanPath){
        $plan=Read-GuardJson $ChangePlanPath
        if((Get-GuardProperty $plan 'schema') -ne 'singularityvr-change-plan-v1' -or
           (Get-GuardProperty $plan 'profileSha256') -ine $profileHash -or -not (Get-GuardProperty $plan 'reason')){
            throw 'Change plan is stale, unbound or missing its reason'
        }
        foreach($entry in @(Get-GuardProperty $plan 'files')){
            $path=ConvertTo-GuardRelative (Get-GuardProperty $entry 'path')
            $before=Get-GuardProperty $entry 'before';$after=Get-GuardProperty $entry 'after'
            Assert-GuardHash $before;Assert-GuardHash $after
            if($null -ne $before){$before=$before.ToUpperInvariant()}
            if($null -ne $after){$after=$after.ToUpperInvariant()}
            $entry.before=$before;$entry.after=$after
            if(-not $baseFiles.ContainsKey($path) -or $planFiles.ContainsKey($path) -or
               -not (Test-GuardSame $before $baseFiles[$path]) -or (Test-GuardSame $before $after) -or
               -not (Get-GuardProperty $entry 'reason')){throw "Invalid exact file allowance: $path"}
            $planFiles[$path]=$entry
        }
        foreach($entry in @(Get-GuardProperty $plan 'ini')){
            $key=Get-GuardProperty $entry 'key';$before=Get-GuardProperty $entry 'before';$after=Get-GuardProperty $entry 'after'
            if($key -notmatch '^[^/]+/[^/]+$' -or $key -match '[*?\[\]]' -or $planIni.ContainsKey($key) -or
               $key -in $ignore -or -not (Test-GuardSame $before $baseIni[$key]) -or
               (Test-GuardSame $before $after) -or -not (Get-GuardProperty $entry 'reason') -or
               ($null -ne $before -and $before -isnot [string]) -or ($null -ne $after -and $after -isnot [string])){
                throw "Invalid exact INI allowance: $key"
            }
            $planIni[$key]=$entry
        }
    }
    $nextFiles=$baseFiles.Clone();$nextIni=$baseIni.Clone()
    foreach($key in $planFiles.Keys){$nextFiles[$key]=$planFiles[$key].after}
    foreach($key in $planIni.Keys){
        if($null -eq $planIni[$key].after){$nextIni.Remove($key)}else{$nextIni[$key]=$planIni[$key].after}
    }
    foreach($key in $dependencies.Keys){if($nextIni[$key] -eq '1'){
        foreach($path in $dependencies[$key]){if(-not $nextFiles.ContainsKey($path) -or $null -eq $nextFiles[$path]){
            throw "Candidate enabled feature dependency is not pinned: $path"
        }}
    }}
    $forward=($Direction -eq 'Install')
    $beforeFiles=if($forward){$baseFiles}else{$nextFiles}
    $afterFiles=if($forward){$nextFiles}else{$baseFiles}
    $beforeIni=if($forward){$baseIni}else{$nextIni}
    $afterIni=if($forward){$nextIni}else{$baseIni}
    $map=@{};$sources=@{};$preimages=@{}
    foreach($entry in $FileMap){
        $target=ConvertTo-GuardRelative (Get-GuardProperty $entry 'Target')
        $source=Get-GuardProperty $entry 'Source'
        if($map.ContainsKey($target) -or ($target -ine $iniRelative -and -not $baseFiles.ContainsKey($target))){
            throw "Unlisted/duplicate deployment target: $target"
        }
        if($source -and -not (Test-Path -LiteralPath $source -PathType Leaf)){throw "Missing planned source: $source"}
        $map[$target]=$source
        $absolute=Resolve-GuardPath $GameRoot $target
        $sources[$absolute]=if($source){Get-GuardHash $source}else{$null}
        $preimages[$absolute]=Get-GuardHash $absolute
        if($target -ine $iniRelative -and -not (Test-GuardSame $sources[$absolute] $afterFiles[$target])){
            throw "Candidate file is not approved: $target"
        }
    }
    foreach($key in $planFiles.Keys){if(-not $map.ContainsKey($key)){throw "Unused file allowance: $key"}}
    if($planIni.Count -and -not $map.ContainsKey($iniRelative)){throw 'INI allowances without an INI write'}
    $expectedFiles=if($Phase -eq 'Pre'){$beforeFiles}else{$afterFiles}
    $expectedIni=if($Phase -eq 'Pre'){$beforeIni}else{$afterIni}
    foreach($path in $baseFiles.Keys){
        $actual=Get-GuardHash (Resolve-GuardPath $GameRoot $path)
        if(-not (Test-GuardSame $actual $expectedFiles[$path])){throw "Protected file drift ($Phase): $path"}
    }
    $liveIni=Read-GuardIni $iniPath
    foreach($key in @(@($liveIni.Keys)+@($expectedIni.Keys) | Sort-Object -Unique)){
        if($key -notin $ignore -and -not (Test-GuardSame $liveIni[$key] $expectedIni[$key])){
            throw "Protected INI drift ($Phase): $key expected='$($expectedIni[$key])' actual='$($liveIni[$key])'"
        }
    }
    if($map.ContainsKey($iniRelative)){
        if(-not $map[$iniRelative]){throw 'Deleting the INI is forbidden'}
        $candidateIni=Read-GuardIni $map[$iniRelative]
        foreach($key in @(@($candidateIni.Keys)+@($afterIni.Keys) | Sort-Object -Unique)){
            if($key -notin $ignore -and -not (Test-GuardSame $candidateIni[$key] $afterIni[$key])){
                throw "Candidate INI not approved: $key"
            }
        }
    }
    return [pscustomobject]@{ProfileSha256=$profileHash;ProfilePath=$ProfilePath;GameRoot=$GameRoot;
        PlanSha256=if($ChangePlanPath){Get-GuardHash $ChangePlanPath}else{$null};PlanPath=$ChangePlanPath;
        Sources=$sources;Preimages=$preimages;Direction=$Direction;Phase=$Phase}
}

function Assert-GuardWriteTicket($Ticket,[string]$Target,[string]$Source) {
    if($null -eq $Ticket){throw 'No accepted-profile preflight ticket; refusing write'}
    $full=[IO.Path]::GetFullPath($Target)
    if(-not $Ticket.Sources.ContainsKey($full)){throw "Write outside preflight map: $full"}
    $prefix=[IO.Path]::GetFullPath($Ticket.GameRoot).TrimEnd('\','/')+[IO.Path]::DirectorySeparatorChar
    if(-not $full.StartsWith($prefix,[StringComparison]::OrdinalIgnoreCase)){throw 'Ticket target escaped root'}
    [void](Resolve-GuardPath $Ticket.GameRoot $full.Substring($prefix.Length))
    if((Get-GuardHash $Ticket.ProfilePath) -ne $Ticket.ProfileSha256 -or
       ($Ticket.PlanPath -and (Get-GuardHash $Ticket.PlanPath) -ne $Ticket.PlanSha256)){
        throw 'Profile/plan changed after preflight'
    }
    $actualSource=if($Source){Get-GuardHash $Source}else{$null}
    if(-not (Test-GuardSame $actualSource $Ticket.Sources[$full])){throw 'Source changed after preflight'}
    if(-not (Test-GuardSame (Get-GuardHash $full) $Ticket.Preimages[$full])){throw 'Target changed after preflight'}
}

function New-AcceptedProfileDraft {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$GameRoot,
          [Parameter(Mandatory)][string[]]$RelativeFiles,
          [string]$IniRelativePath='Binaries/SingularityVR.ini')
    # Explicit inventory only; no auto promotion, recursive discovery or writes.
    $pins=@();$seen=@{}
    foreach($relative in $RelativeFiles){
        $safe=ConvertTo-GuardRelative $relative
        if($seen.ContainsKey($safe)){throw "Duplicate draft inventory path: $safe"}
        $seen[$safe]=$true
        $pins += [pscustomobject]@{path=$safe;sha256=(Get-GuardHash (Resolve-GuardPath $GameRoot $safe))}
    }
    return [pscustomobject]@{schema='singularityvr-accepted-profile-v1';approved=$false;
        acceptedBy='';acceptanceScope='';iniPath=(ConvertTo-GuardRelative $IniRelativePath);
        ini=(Read-GuardIni (Resolve-GuardPath $GameRoot $IniRelativePath));
        ignoredIniKeys=@('Render/AutoResX','Render/AutoResY');files=$pins}
}
