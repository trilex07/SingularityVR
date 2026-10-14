[CmdletBinding(SupportsShouldProcess)]
param([Parameter(Mandatory)][string]$OutputDirectory,[switch]$Apply)
# MAIN cleanup for this exact completed trial. Preserve the user's two UI changes.
Set-StrictMode -Version Latest;$ErrorActionPreference='Stop'
$repo=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$game='C:\Program Files (x86)\Steam\steamapps\common\Singularity'
$target=Join-Path $game 'Binaries/SingularityVR.ini'
$saved=Join-Path $repo 'deployment/hands-dubbing/backups/20261014-162546-8912/SingularityVR.ini'
$staged=Join-Path $repo 'deployment/hands-dubbing/post-opening/next-core-v1/SingularityVR.ini'
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(-not $out.StartsWith($repo+'\deployment\hands-dubbing\',[StringComparison]::OrdinalIgnoreCase) -or (Test-Path $out)){throw 'Fresh owned output required'}
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash}
if((Hash $saved) -cne 'B3FEED43E7651E86C57B3767359F13C363B2F3BBFE067429E29F81F4CB2A079F' -or
 (Hash $staged) -cne 'D1D91B81641A2F3186F2F5782DCF3827D6BD4A8BECBA52085501606CCE9E0621'){throw 'Known three-way base changed'}
if((Hash $target) -cne '34D18857B3E5020A54AD645C40523CA874C61BC685BA06A9B67D95C1F0489AF0'){throw 'New user INI changes; refuse automatic cleanup'}
$encoding=[Text.Encoding]::GetEncoding(28591)
$base=$encoding.GetString([IO.File]::ReadAllBytes($saved))
$candidate=$encoding.GetString([IO.File]::ReadAllBytes($staged))
$current=$encoding.GetString([IO.File]::ReadAllBytes($target))
if($current -cne $candidate.Replace('ShowControllerArms=0','ShowControllerArms=1').Replace('UiNarratorEnabled=1','UiNarratorEnabled=0')){throw 'Unknown user delta'}
$nl=if($base.Contains("`r`n")){"`r`n"}else{"`n"}
if([regex]::Matches($base,'(?m)^\[Localization\](?=\r?$)').Count -ne 1 -or $base -match 'UiNarratorEnabled'){throw 'Unknown baseline layout'}
$merged=$base.Replace('[Localization]','[Localization]'+$nl+'UiNarratorEnabled=0')
if($merged -match '(?m)^(CutsceneWorldScreen|CutsceneScreenDistanceMm|NoTmdPaletteFix|NativePolishSpeech|NoTmdWristAnchorFix|TextureBridgeOwned|ModalUiProbe|HandAnchorProbe|CutsceneMenuHandoffFix|CutsceneMenuPairFix)='){throw 'Owned experiment cleanup incomplete'}
New-Item -ItemType Directory -Path $out | Out-Null
Copy-Item -LiteralPath $target -Destination (Join-Path $out 'user-after-test.ini')
$prepared=Join-Path $out 'safe-preserving-user.ini'
[IO.File]::WriteAllBytes($prepared,$encoding.GetBytes($merged))
$newHash=Hash $prepared
[ordered]@{OriginalSha256=(Hash $saved);UserAfterTestSha256=(Hash $target);PreparedSha256=$newHash;Preserved='ShowControllerArms=1; UiNarratorEnabled=0';Removed='Only MAIN staged trial flags';Applied=$false}|ConvertTo-Json|Set-Content (Join-Path $out 'plan.json') -Encoding UTF8
Write-Output "PREPARED: baseline INI plus user narratorOFF; hash=$newHash"
if($Apply -and $PSCmdlet.ShouldProcess($target,'Remove owned trial switches, preserve user armsON/narratorOFF')){
 if(-not(Get-CimInstance Win32_Process -Filter "ProcessId=$PID") -or @(Get-Process Singularity -ErrorAction SilentlyContinue).Count){throw 'Full view and closed game required'}
 if((Hash $target) -cne '34D18857B3E5020A54AD645C40523CA874C61BC685BA06A9B67D95C1F0489AF0'){throw 'INI changed during preparation'}
 $temp=$target+".reconcile-$PID.new"
 if(Test-Path $temp){throw 'Existing temporary file'}
 try{
  Copy-Item -LiteralPath $prepared -Destination $temp
  if((Hash $temp) -cne $newHash){throw 'Temporary hash mismatch'}
  if(@(Get-Process Singularity -ErrorAction SilentlyContinue).Count){throw 'Game started; no replacement'}
  Move-Item -LiteralPath $temp -Destination $target -Force
  if((Hash $target) -cne $newHash){throw 'Installed reconciliation mismatch'}
 }finally{if(Test-Path $temp){Remove-Item -LiteralPath $temp}}
 Write-Output "RECONCILED: user changes retained, owned test flags absent; hash=$newHash"
}
