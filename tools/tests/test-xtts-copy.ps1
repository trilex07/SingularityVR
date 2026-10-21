$ErrorActionPreference='Stop'
# Execute only these repo-owned helper definitions, not the installer's main.
$tokens=$null;$errors=$null
$source=Join-Path $PSScriptRoot '../polish/install-xtts-audio-candidate.ps1'
$ast=[Management.Automation.Language.Parser]::ParseFile($source,[ref]$tokens,[ref]$errors)
if($errors.Count){throw 'Installer parse failed'}
foreach($name in @('Hash','Copy-Verified')){
    $node=$ast.FindAll({param($n) $n -is [Management.Automation.Language.FunctionDefinitionAst]},$false)|Where-Object Name -eq $name
    if(@($node).Count -ne 1){throw 'Helper identity missing'}
    . ([scriptblock]::Create($node.Extent.Text))
}
function Assert-Closed {if(-not $script:closed){throw 'simulated running game'}}
$script:closed=$true
$root=Join-Path ([IO.Path]::GetTempPath()) ('svr-xtts-copy-'+[guid]::NewGuid().ToString('N'))
$null=New-Item -ItemType Directory -Path $root
$src=Join-Path $root 'source.wav';$dst=Join-Path $root 'target.wav'
try{
    [IO.File]::WriteAllBytes($src,[byte[]](1,2,3,4))
    [IO.File]::WriteAllBytes($dst,[byte[]](8,9))
    $expected=Hash $src
    Copy-Verified $src $dst $expected
    if((Hash $dst) -cne $expected){throw 'Existing destination was not replaced'}
    Remove-Item -LiteralPath $dst
    Copy-Verified $src $dst $expected
    if((Hash $dst) -cne $expected){throw 'New destination failed'}
    $failed=$false
    try{Copy-Verified $src $dst ('0'*64)}catch{$failed=$true}
    if(-not $failed -or (Hash $dst) -cne $expected){throw 'Hash refusal failed'}
    $script:closed=$false;$failed=$false
    try{Copy-Verified $src $dst $expected}catch{$failed=$true}
    if(-not $failed -or (Hash $dst) -cne $expected){throw 'Running-game refusal failed'}
    Write-Output 'PASS: existing/new destination, bad hash, running-game refusal (synthetic files only)'
}finally{
    foreach($path in @($src,$dst,($dst+".xtts-$PID.new"))){if(Test-Path -LiteralPath $path){Remove-Item -LiteralPath $path}}
    Remove-Item -LiteralPath $root
}
