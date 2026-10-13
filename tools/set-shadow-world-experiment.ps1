[CmdletBinding()]
param([Parameter(Mandatory)][ValidateSet('ON','OFF')][string]$State)
$ErrorActionPreference='Stop'
$flag=Join-Path $env:LOCALAPPDATA 'SingularityVR\shadow-world-receiver.flag'
$payload='SingularityVR world shadow reconstruction v1'
if((Test-Path -LiteralPath $flag) -and [IO.File]::ReadAllText($flag) -ne $payload){
    throw 'Unknown world-shadow flag; refusing overwrite/removal'
}
if($State -eq 'ON'){
    [IO.File]::WriteAllText($flag,$payload,[Text.Encoding]::ASCII)
    'WORLD SHADOW EXPERIMENT ON: world-depth only, 20 minute lease'
}elseif(Test-Path -LiteralPath $flag){
    Remove-Item -LiteralPath $flag
    'WORLD SHADOW EXPERIMENT OFF: removed owned flag'
}
