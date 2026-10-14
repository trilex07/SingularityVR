[CmdletBinding()]
param([Parameter(Mandatory)][ValidateSet('ON','OFF')][string]$State,
      [string]$IniPath='C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\SingularityVR.ini',
      [string]$LeaseDirectory='',
      [switch]$Apply)
# Adds or removes only Diagnostic.MenuAssetWitness. No other INI key is rewritten.
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'

function Get-WitnessText([string]$Path) {
    $bytes=[IO.File]::ReadAllBytes($Path)
    $text=[Text.Encoding]::GetEncoding(28591).GetString($bytes)
    if($text.Contains([char]0)){throw 'Unexpected INI encoding'}
    return $text
}
function Add-MenuAssetWitness([string]$Text) {
    if($Text -cmatch 'MenuAssetWitness'){throw 'Existing witness key'}
    $diag=[regex]::Matches($Text,'(?m)^\[Diagnostic\](?=\r?$)')
    if($diag.Count -ne 1){throw 'Exactly one Diagnostic section required'}
    $ending=if($Text.Contains("`r`n")){"`r`n"}else{"`n"}
    $at=$diag[0].Index+$diag[0].Length
    $added=$ending+'MenuAssetWitness=1'
    $next=$Text.Insert($at,$added)
    if($next.Remove($at,$added.Length) -cne $Text){throw 'Unowned diagnostic edit'}
    return $next
}
function Remove-MenuAssetWitness([string]$Text) {
    $ending=if($Text.Contains("`r`n")){"`r`n"}else{"`n"}
    $line=$ending+'MenuAssetWitness=1'
    if(([regex]::Matches($Text,[regex]::Escape($line))).Count -ne 1){throw 'Witness line is not the single owned key'}
    return $Text.Replace($line,'')
}

if(-not $Apply){
    Write-Output 'DRY: functions loaded; pass -Apply to edit an INI'
    return
}
if(@(Get-Process -Name Singularity -ErrorAction SilentlyContinue).Count){throw 'Singularity is running; refusing INI edit'}
$encoding=[Text.Encoding]::GetEncoding(28591)
if($State -eq 'ON'){
    if(-not $LeaseDirectory){throw 'ON requires -LeaseDirectory'}
    $lease=[IO.Path]::GetFullPath($LeaseDirectory)
    if(Test-Path -LiteralPath $lease){throw 'Refuse existing lease directory'}
    $text=Get-WitnessText $IniPath
    $next=Add-MenuAssetWitness $text
    New-Item -ItemType Directory -Path $lease | Out-Null
    [IO.File]::WriteAllBytes((Join-Path $lease 'SingularityVR.ini.original'),$encoding.GetBytes($text))
    [IO.File]::WriteAllBytes($IniPath,$encoding.GetBytes($next))
    $candidate=(Get-FileHash -LiteralPath $IniPath -Algorithm SHA256).Hash
    @{State='ON';OriginalSha256=(Get-FileHash -LiteralPath (Join-Path $lease 'SingularityVR.ini.original')).Hash;CandidateSha256=$candidate;OwnedKey='Diagnostic.MenuAssetWitness=1'} |
        ConvertTo-Json | Set-Content -LiteralPath (Join-Path $lease 'ini-lease.json') -Encoding UTF8
    Write-Output "WITNESS INI ON: $candidate"
} else {
    if(-not $LeaseDirectory){throw 'OFF requires -LeaseDirectory'}
    $lease=[IO.Path]::GetFullPath($LeaseDirectory)
    $saved=Get-WitnessText (Join-Path $lease 'SingularityVR.ini.original')
    $current=Get-WitnessText $IniPath
    $record=Get-Content -LiteralPath (Join-Path $lease 'ini-lease.json') -Raw | ConvertFrom-Json
    if((Get-FileHash -LiteralPath $IniPath).Hash -eq $record.CandidateSha256){
        [IO.File]::WriteAllBytes($IniPath,$encoding.GetBytes($saved))
    } else {
        $stripped=Remove-MenuAssetWitness $current
        if($stripped -eq $current){throw 'Witness removal changed nothing'}
        [IO.File]::WriteAllBytes($IniPath,$encoding.GetBytes($stripped))
        Write-Output 'WITNESS INI: user delta preserved; only MenuAssetWitness removed'
    }
    if((Get-WitnessText $IniPath) -cmatch 'MenuAssetWitness'){throw 'Witness key survived'}
    Write-Output ("WITNESS INI OFF: {0}" -f (Get-FileHash -LiteralPath $IniPath).Hash)
}
