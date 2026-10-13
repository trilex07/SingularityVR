[CmdletBinding()]
param([Parameter(Mandatory)][ValidateSet('ON','OFF')][string]$State,
      [ValidateSet('Receiver','ClearGate')][string]$Experiment='Receiver')
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$flag='C:\Users\Trilex\AppData\Local\SingularityVR\shadow-receiver-fix.flag'
$payload='SingularityVR shadow receiver experiment v1'
if($Experiment -eq 'ClearGate') {
    $flag='C:\Users\Trilex\AppData\Local\SingularityVR\shadow-clear-gate.flag'
    $payload='SingularityVR shadow clear gate experiment v1'
}
if (Test-Path -LiteralPath $flag) {
    if ([IO.File]::ReadAllText($flag) -ne $payload) { throw 'Unknown flag contents; refusing overwrite/removal' }
}
if ($State -eq 'ON') {
    [IO.File]::WriteAllText($flag,$payload,[Text.Encoding]::ASCII)
    Write-Output "EXPERIMENT ON (20 minute lease): $flag"
} elseif(Test-Path -LiteralPath $flag) {
    Remove-Item -LiteralPath $flag
    Write-Output 'EXPERIMENT OFF: removed owned flag; no game files changed'
}
