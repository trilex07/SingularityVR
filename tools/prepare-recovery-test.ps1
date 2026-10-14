[CmdletBinding()]
param([Parameter(Mandatory)][string]$OutputDirectory,
      [Parameter(Mandatory)][string]$VrDll,
      [Parameter(Mandatory)][string]$AcceptedCoreManifest,
      [Parameter(Mandatory)][string]$AcceptedAssetManifest,
      [string]$GameRoot='C:\Program Files (x86)\Steam\steamapps\common\Singularity')
# Offline only: combine the accepted first-area payload with the current ring/title.
# No installation, launch, arbitrary adoption of unknown installed files, or save access.
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
function Hash([string]$p){if(Test-Path -LiteralPath $p -PathType Leaf){return (Get-FileHash -LiteralPath $p).Hash};return 'ABSENT'}
function Child([string]$root,[string]$relative){
 $base=[IO.Path]::GetFullPath($root).TrimEnd('\','/')
 $p=[IO.Path]::GetFullPath((Join-Path $base $relative))
 if(-not $p.StartsWith($base+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Path escape'}
 return $p
}
. (Join-Path $PSScriptRoot 'recovery-ini.ps1')
$corePath=(Resolve-Path -LiteralPath $AcceptedCoreManifest).Path
$assetsPath=(Resolve-Path -LiteralPath $AcceptedAssetManifest).Path
if((Hash $corePath) -cne 'F7B174C7371A39BC289672D456E72A371B9B86EC3DBFC515E0F577C7C4A878AD' -or
   (Hash $assetsPath) -cne 'D6BDCB751AB4EA786D2DBD8F78B92097E841659B6432D26DCBAA2BE4563A01DD'){
 throw 'Not the recorded physically accepted first-area manifests'
}
$core=Get-Content -LiteralPath $corePath -Raw | ConvertFrom-Json
$assets=Get-Content -LiteralPath $assetsPath -Raw | ConvertFrom-Json
$pins=@{
 'Binaries\d3d9.dll'='A8F38EF6007BCED8F3CB247E5A4A65942958CD4A1ED43F1B6601F81E342D001C'
 'Binaries\dinput8.dll'='AAA07005F4FF658C4A71CC30EA258E469DAF250C77BFE447011FFD650379E4F7'
 'Binaries\SingularityVR.ini'='C3E5197262C37D340F81487F3E0778AA87DB10891A7DFCA4988A2EFBB85D2C14'
 'RvGame\CookedPC\SP_VI_intro_Kismet_LOC_INT.xxx'='890C61720ABC0BB5AFAD86F16433A811A6F21BA6FC6284563BC5875E52642E85'
}
$ui='RvGame\CookedPC\UI_SinglePlayer_SF.xxx'
$uiHash='69CABCCF13C0F13B120163331ADDCFDDE3764CE8E32FE60A15F800FDF8C87D66'
$out=[IO.Path]::GetFullPath($OutputDirectory)
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if(-not $out.StartsWith($repo+'\',[StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $out)){
 throw 'Output must be a new directory inside this worktree, outside Steam'
}
if(@($core.Files).Count -ne 4 -or @($assets.Files).Count -ne 38){throw 'Wrong accepted profile'}
foreach($f in $core.Files){
 if(-not $pins.ContainsKey($f.RelativePath) -or (Hash (Child $GameRoot $f.RelativePath)) -cne $pins[$f.RelativePath]){throw "Current core changed: $($f.RelativePath)"}
 if((Hash (Child (Split-Path $corePath) $f.Source)) -cne $f.CandidateSha256){throw 'Accepted core source changed'}
}
foreach($f in $assets.Files){
 $expected=if($f.RelativePath -ceq $ui){$uiHash}else{$f.BaselineSha256}
 if((Hash (Child $GameRoot $f.RelativePath)) -cne $expected){throw "Current asset changed: $($f.RelativePath)"}
 if((Hash (Child (Split-Path $assetsPath) $f.Source)) -cne $f.CandidateSha256){throw 'Accepted asset source changed'}
}
$build=Get-Content -LiteralPath ([IO.Path]::ChangeExtension($VrDll,'build.json')) -Raw | ConvertFrom-Json
if($build.sha256 -cne (Hash $VrDll) -or $build.architecture -cne 'x86 PE32' -or $build.optimization -cne '/O2'){
 throw 'Unverified optimized x86 build'
}
$coreOut=Join-Path $out 'core';$assetOut=Join-Path $out 'assets'
New-Item -ItemType Directory -Path $coreOut,$assetOut | Out-Null
$encoding=[Text.Encoding]::GetEncoding(28591)
$ini=$encoding.GetString([IO.File]::ReadAllBytes((Child $GameRoot 'Binaries\SingularityVR.ini')))
$changes=[ordered]@{NoTmdPaletteFix='1';NoTmdWristAnchorFix='1';CutsceneMenuHandoffFix='1';
 CutsceneMenuPairFix='1';CutsceneWorldScreen='1';CutsceneScreenDistanceMm='2000';TextureBridgeOwned='1';
 MenuOverlay='1';DesktopMirrorEye='2';ShowControllerArms='0';UiElementDump='0';PauseMenuDiag='0'}
foreach($key in $changes.Keys){$ini=Set-RecoveryIniKey $ini 'Render' $key $changes[$key]}
$ini=Set-RecoveryIniKey $ini 'Localization' 'NativePolishSpeech' '1'
$ini=Set-RecoveryIniKey $ini 'Diagnostic' 'IndependentHandsAudit' '0'
$ini=Set-RecoveryIniKey $ini 'Diagnostic' 'MenuAssetWitness' '0'
# Deliberately preserve the user's narrator OFF, recenter and calibration choices.
foreach($f in $core.Files){
 $source=if($f.RelativePath -ceq 'Binaries\d3d9.dll'){$VrDll}else{Child (Split-Path $corePath) $f.Source}
 $target=Child $coreOut $f.Source
 if($f.RelativePath -ceq 'Binaries\SingularityVR.ini'){[IO.File]::WriteAllBytes($target,$encoding.GetBytes($ini))}
 else{Copy-Item -LiteralPath $source -Destination $target}
 $f.BaselineSha256=$pins[$f.RelativePath];$f.CandidateSha256=Hash $target
}
$core.FrozenUiSha256=$uiHash
$core.IniChangeOnly='Recovery of accepted feature flags; native RTT overlay ON; TV aspect-fit; diagnostics OFF; user narrator/recenter/calibration retained'
$core.PhysicalAcceptance='PENDING'
$core | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $coreOut 'test-manifest.json') -Encoding UTF8
foreach($f in $assets.Files){
 $source=if($f.RelativePath -ceq $ui){Child $GameRoot $ui}else{Child (Split-Path $assetsPath) $f.Source}
 $target=Child $assetOut $f.Source
 New-Item -ItemType Directory -Path (Split-Path $target) -Force | Out-Null
 Copy-Item -LiteralPath $source -Destination $target
 if($f.RelativePath -ceq $ui){$f.BaselineSha256=$uiHash;$f.CandidateSha256=$uiHash}
 if((Hash $target) -cne $f.CandidateSha256){throw 'Staging changed candidate content'}
}
$assets | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $assetOut 'asset-manifest.json') -Encoding UTF8
Copy-Item -LiteralPath ([IO.Path]::ChangeExtension($VrDll,'build.json')) -Destination (Join-Path $out 'd3d9.build.json')
Write-Output "OFFLINE STAGED: $out; exact core4+asset38; no Steam writes/launch; start required"
