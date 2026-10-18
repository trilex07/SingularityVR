[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$InputIni,
    [Parameter(Mandatory)][string]$OutputIni,
    [switch]$RightEyeMirror,
    [switch]$OpaqueXrCapture
)

Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$inputFull=(Resolve-Path -LiteralPath $InputIni).Path
$outputFull=[IO.Path]::GetFullPath($OutputIni)
if([string]::Equals($inputFull,$outputFull,[StringComparison]::OrdinalIgnoreCase)){
    throw 'Candidate INI must not overwrite its source'
}
if($outputFull -match '(?i)steamapps[\\/]common[\\/]singularity|program files'){
    throw 'Candidate INI output must stay outside the game installation'
}
if(Test-Path -LiteralPath $outputFull){throw 'Refusing to overwrite an existing candidate INI'}
if(-not(Test-Path -LiteralPath (Split-Path -Parent $outputFull))){throw 'Candidate directory must already exist'}

$utf8=[Text.UTF8Encoding]::new($false,$true)
$text=$utf8.GetString([IO.File]::ReadAllBytes($inputFull))
$newline=if($text.Contains("`r`n")){"`r`n"}elseif($text.Contains("`r")){"`r"}else{"`n"}
$endsWithNewline=$text.EndsWith("`n") -or $text.EndsWith("`r")
$lines=[Collections.Generic.List[string]]::new()
foreach($line in [regex]::Split($text,'\r\n|\n|\r')){$lines.Add($line)}
if($endsWithNewline -and $lines.Count -gt 0 -and $lines[$lines.Count-1] -eq ''){$lines.RemoveAt($lines.Count-1)}

# The menu test previously inherited a pre-VFX rollback INI. The code was
# present but default-OFF, silently disabling the user's accepted muzzle fix.
# Keep its physical acceptance separate from the historical restore hash.
$targets=@{ContextPointerProjection='1';ContextCursorSuppress='0';ContextCursorNative='1';NativeMuzzleParticleStereo='1'}
if($OpaqueXrCapture){$targets.XrCaptureOpaqueAlpha='1'}
if($RightEyeMirror){
    $targets.DesktopMirrorEye='2'
    $targets.DesktopMirrorFill='0'
    $targets.DesktopMirrorWindow='1'
    $targets.SteamCaptureBackbuffer='1'
}
$seen=@{}
foreach($key in $targets.Keys){$seen[$key]=0}
$insideRender=$false
$renderHeader=-1
for($i=0;$i -lt $lines.Count;$i++){
    if($lines[$i] -match '^\s*\[([^\]]+)\]\s*$'){
        $insideRender=$Matches[1] -ieq 'Render'
        if($insideRender){$renderHeader=$i}
        continue
    }
    if(-not $insideRender){continue}
    foreach($key in $targets.Keys){
        if($lines[$i] -match ('^\s*'+[regex]::Escape($key)+'\s*=')){
            $seen[$key]++
            if($seen[$key] -gt 1){throw "Duplicate [Render] key: $key"}
            $lines[$i]="$key=$($targets[$key])"
        }
    }
}
if($renderHeader -lt 0){throw 'Input INI has no [Render] section'}
for($i=$targets.Keys.Count-1;$i -ge 0;$i--){
    $key=@($targets.Keys)[$i]
    if($seen[$key] -eq 0){$lines.Insert($renderHeader+1,"$key=$($targets[$key])")}
}
$candidate=($lines -join $newline)
if($endsWithNewline){$candidate+=$newline}
[IO.File]::WriteAllText($outputFull,$candidate,$utf8)

# Verify the result differs only in the explicitly scoped [Render] values.
$checkText=$utf8.GetString([IO.File]::ReadAllBytes($outputFull))
$checkLines=[regex]::Split($checkText,'\r\n|\n|\r')
$checkSection='';$values=@{}
foreach($line in $checkLines){
    if($line -match '^\s*\[([^\]]+)\]\s*$'){$checkSection=$Matches[1];continue}
    if($line -match '^\s*[;#]'){continue}
    if($checkSection -and $line -match '^\s*([^=]+?)\s*=\s*(.*?)\s*$'){
        $name=$checkSection+'/'+$Matches[1].Trim()
        if($values.ContainsKey($name)){throw "Duplicate candidate INI key: $name"}
        $values[$name]=$Matches[2]
    }
}
foreach($key in $targets.Keys){if($values['Render/'+$key] -cne $targets[$key]){throw "Candidate setting verification failed: $key"}}
$before=[Collections.Generic.List[string]]::new()
foreach($line in [regex]::Split($text,'\r\n|\n|\r')){$before.Add($line)}
$beforeSections=@{};$section=''
foreach($line in $before){
    if($line -match '^\s*\[([^\]]+)\]\s*$'){$section=$Matches[1];continue}
    if($line -match '^\s*[;#]'){continue}
    if($section -and $line -match '^\s*([^=]+?)\s*=\s*(.*?)\s*$'){
        $name=$section+'/'+$Matches[1].Trim()
        if($beforeSections.ContainsKey($name)){throw "Duplicate input INI key: $name"}
        $beforeSections[$name]=$Matches[2]
    }
}
foreach($key in $beforeSections.Keys){
    if($key.StartsWith('Render/') -and $targets.ContainsKey($key.Substring(7))){continue}
    if(-not $values.ContainsKey($key) -or $values[$key] -cne $beforeSections[$key]){throw "Unrelated INI value changed: $key"}
}
Write-Output "Prepared outside-install candidate INI: $outputFull"
foreach($key in $targets.Keys){Write-Output ('Scoped candidate value Render/'+$key+'='+$targets[$key])}
