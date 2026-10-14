[CmdletBinding()]
param()
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$repo=(Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$tool=Join-Path $repo 'tools\set-menu-asset-witness.ps1'
$source='C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\SingularityVR.ini'
$root=Join-Path $repo 'deployment\hands-dubbing\menu-witness-ini-test'
if(Test-Path -LiteralPath $root){Remove-Item -LiteralPath $root -Recurse -Force}
New-Item -ItemType Directory -Path $root | Out-Null
$ini=Join-Path $root 'SingularityVR.ini'
$lease=Join-Path $root 'lease'
Copy-Item -LiteralPath $source -Destination $ini
$before=(Get-FileHash -LiteralPath $ini).Hash
& $tool -State ON -IniPath $ini -LeaseDirectory $lease -Apply
if((Get-Content -LiteralPath $ini -Raw) -notmatch '(?m)^MenuAssetWitness=1\r?$'){throw 'Witness key missing'}
if(((Get-Content -LiteralPath $ini -Raw) -split 'MenuAssetWitness').Count -ne 2){throw 'Witness key repeated'}
& $tool -State OFF -IniPath $ini -LeaseDirectory $lease -Apply
if((Get-FileHash -LiteralPath $ini).Hash -cne $before){throw 'Exact original INI not restored'}
$lease2=Join-Path $root 'lease-user'
Copy-Item -LiteralPath $source -Destination $ini -Force
& $tool -State ON -IniPath $ini -LeaseDirectory $lease2 -Apply
Add-Content -LiteralPath $ini -Value 'UserNote=1' -Encoding ascii
& $tool -State OFF -IniPath $ini -LeaseDirectory $lease2 -Apply
$text=Get-Content -LiteralPath $ini -Raw
if($text -match 'MenuAssetWitness'){throw 'User-delta path left the witness key'}
if($text -notmatch 'UserNote=1'){throw 'User-delta path dropped the extra line'}
Write-Output 'PASS: witness INI exact restore and user-delta removal'
