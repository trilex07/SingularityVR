[CmdletBinding()]
param([Parameter(Mandatory)][ValidateSet('ON','OFF')][string]$State,
      [ValidateSet('World','FirstPerson')][string]$Experiment='World')
$ErrorActionPreference='Stop'
$flag=Join-Path $env:LOCALAPPDATA 'SingularityVR\shadow-world-receiver.flag'
$payload='SingularityVR world shadow reconstruction v1'
if($Experiment -eq 'FirstPerson'){
    $flag=Join-Path $env:LOCALAPPDATA 'SingularityVR\shadow-first-person-receiver.flag'
    $payload='SingularityVR first person shadow reconstruction v1'
}
if((Test-Path -LiteralPath $flag) -and [IO.File]::ReadAllText($flag) -ne $payload){
    throw 'Unknown world-shadow flag; refusing overwrite/removal'
}
if($State -eq 'ON'){
    [IO.File]::WriteAllText($flag,$payload,[Text.Encoding]::ASCII)
    "SHADOW $Experiment EXPERIMENT ON: isolated depth family, 20 minute lease"
}elseif(Test-Path -LiteralPath $flag){
    Remove-Item -LiteralPath $flag
    "SHADOW $Experiment EXPERIMENT OFF: removed owned flag"
}
