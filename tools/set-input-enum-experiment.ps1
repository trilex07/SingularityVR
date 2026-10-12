[CmdletBinding()]
param([Parameter(Mandatory)][ValidateSet('ON','OFF')][string]$State)
$ErrorActionPreference='Stop'
$flag='C:\Users\Trilex\AppData\Local\SingularityVR\input-enum-throttle.flag'
$payload='SingularityVR input enum throttle v1'
if((Test-Path -LiteralPath $flag) -and [IO.File]::ReadAllText($flag) -ne $payload) {
    throw 'Unknown experiment flag contents; refusing overwrite or removal'
}
if($State -eq 'ON') {
    [IO.File]::WriteAllText($flag,$payload,[Text.Encoding]::ASCII)
    'INPUT ENUM EXPERIMENT ON: 20 minute lease, no installed file changes'
} elseif(Test-Path -LiteralPath $flag) {
    Remove-Item -LiteralPath $flag
    'INPUT ENUM EXPERIMENT OFF: removed owned flag'
}
