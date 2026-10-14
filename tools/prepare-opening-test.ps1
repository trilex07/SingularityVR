[CmdletBinding()]
param([Parameter(Mandatory)][string]$OutputDirectory,
      [Parameter(Mandatory)][string]$VrDll,
      [Parameter(Mandatory)][string]$InjectorDll,
      [Parameter(Mandatory)][string]$AudioPackage,
      [switch]$FrontendHandoff,
      [switch]$CinemaMenuPair,
      [switch]$HandAnchorProbe,
      [switch]$WristAnchor,
      [switch]$TextureBridge,
      [switch]$NativeSpeech,
      [switch]$GlovesOnly,
      [switch]$ModalUiProbe,
      [switch]$UiNarrator,
      [switch]$WorldCinema,
      [string]$SteamRoot='C:\Program Files (x86)\Steam')
# Offline staging only. Does not install, arm diagnostics or launch Steam.
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $out){throw 'Refuse existing artifact directory'}
$gameRoot=[IO.Path]::GetFullPath((Join-Path $SteamRoot 'steamapps\common\Singularity'))
$targets=@('Binaries\d3d9.dll','Binaries\dinput8.dll','Binaries\SingularityVR.ini',
           'RvGame\CookedPC\SP_VI_intro_Kismet_LOC_INT.xxx')
$expected=@('D8928180262706A95972D13B728A1867F1EAB1757DCFD6C4A32D99EFBFBEC1BE',
            'AAA07005F4FF658C4A71CC30EA258E469DAF250C77BFE447011FFD650379E4F7',
            'B3FEED43E7651E86C57B3767359F13C363B2F3BBFE067429E29F81F4CB2A079F',
            '890C61720ABC0BB5AFAD86F16433A811A6F21BA6FC6284563BC5875E52642E85')
foreach($i in 0..3) {
    if((Get-FileHash -LiteralPath (Join-Path $gameRoot $targets[$i])).Hash -cne $expected[$i]){
        throw "Baseline changed: $($targets[$i])"
    }
}
$uiHash=(Get-FileHash -LiteralPath (Join-Path $gameRoot 'RvGame\CookedPC\UI_SinglePlayer_SF.xxx')).Hash
if($uiHash -cne '5B2EA469A3EE06FF154AC78BD58604E8034970D4837F289E5C22503C6A673CDE'){throw 'UI changed'}
$iniBytes=[IO.File]::ReadAllBytes((Join-Path $gameRoot $targets[2]))
$encoding=[Text.Encoding]::GetEncoding(28591) # byte-preserving; insert ASCII only
$text=$encoding.GetString($iniBytes)
if($text.Contains([char]0) -or $text -cmatch 'NoTmdPaletteFix'){throw 'Unexpected INI encoding/key'}
$render=[regex]::Matches($text,'(?m)^\[Render\](?=\r?$)')
if($render.Count -ne 1){throw 'Exactly one Render section required'}
$ending=if($text.Contains("`r`n")){"`r`n"}else{"`n"}
$position=$render[0].Index+$render[0].Length
$insert=$ending+'NoTmdPaletteFix=1'
if($FrontendHandoff) {
    if($text -cmatch 'CutsceneMenuHandoffFix'){throw 'Existing frontend experiment key'}
    $insert+=$ending+'CutsceneMenuHandoffFix=1'
}
if($CinemaMenuPair) {
    if(-not $FrontendHandoff -or $text -cmatch 'CutsceneMenuPairFix'){throw 'Menu-pair requires frontend handoff and absent experiment key'}
    $insert+=$ending+'CutsceneMenuPairFix=1'
}
if($HandAnchorProbe){
    if($text -cmatch 'HandAnchorProbe'){throw 'Existing hand-anchor key'}
    $insert+=$ending+'HandAnchorProbe=1'
}
if($WristAnchor){
    if($text -cmatch 'NoTmdWristAnchorFix'){throw 'Existing wrist-anchor key'}
    $insert+=$ending+'NoTmdWristAnchorFix=1'
}
if($TextureBridge){
    if($text -cmatch 'TextureBridgeOwned'){throw 'Existing bridge key'}
    $insert+=$ending+'TextureBridgeOwned=1'
}
if($ModalUiProbe){
    if($text -cmatch 'ModalUiProbe'){throw 'Existing modal probe key'}
    $insert+=$ending+'ModalUiProbe=1'
}
if($WorldCinema){
    if($text -cmatch 'CutsceneWorldScreen|CutsceneScreenDistanceMm'){throw 'Existing cinema anchor experiment key'}
    $insert+=$ending+'CutsceneWorldScreen=1'+$ending+'CutsceneScreenDistanceMm=2000'
}
$candidateIni=$text.Insert($position,$insert)
if($candidateIni.Remove($position,$insert.Length) -cne $text){throw 'INI changed outside owned experiment keys'}
if($NativeSpeech -or $UiNarrator){
    if($candidateIni -cmatch 'NativePolishSpeech'){throw 'Existing native speech key'}
    $loc=[regex]::Matches($candidateIni,'(?m)^\[Localization\](?=\r?$)')
    if($loc.Count -ne 1){throw 'Exactly one Localization section required'}
    $locPosition=$loc[0].Index+$loc[0].Length;$locInsert=''
    if($NativeSpeech){$locInsert+=$ending+'NativePolishSpeech=1'}
    if($UiNarrator){
        if($candidateIni -cmatch 'UiNarratorEnabled'){throw 'Existing UI narration key'}
        $locInsert+=$ending+'UiNarratorEnabled=1'
    }
    $withNative=$candidateIni.Insert($locPosition,$locInsert)
    if($withNative.Remove($locPosition,$locInsert.Length) -cne $candidateIni){throw 'Unowned locale INI change'}
    $candidateIni=$withNative
}
if($GlovesOnly){
    $arms=[regex]::Matches($candidateIni,'(?m)^ShowControllerArms=1(?=\r?$)')
    if($arms.Count -ne 1){throw 'Exactly one observed ShowControllerArms=1 required'}
    $candidateIni=$candidateIni.Remove($arms[0].Index,$arms[0].Length).Insert($arms[0].Index,'ShowControllerArms=0')
}
New-Item -ItemType Directory -Path $out | Out-Null
Copy-Item -LiteralPath $VrDll -Destination (Join-Path $out 'd3d9.dll')
Copy-Item -LiteralPath $InjectorDll -Destination (Join-Path $out 'dinput8.dll')
Copy-Item -LiteralPath $AudioPackage -Destination (Join-Path $out 'SP_VI_intro_Kismet_LOC_INT.xxx')
[IO.File]::WriteAllBytes((Join-Path $out 'SingularityVR.ini'),$encoding.GetBytes($candidateIni))
$files=@()
foreach($i in 0..3) {
    $name=[IO.Path]::GetFileName($targets[$i])
    $files+=@{RelativePath=$targets[$i];Source=$name;BaselineSha256=$expected[$i];
              CandidateSha256=(Get-FileHash -LiteralPath (Join-Path $out $name)).Hash}
}
@{Kind='SingularityVR opening cutscene lease v1';StartRequired=$true;
  FrozenUiSha256=$uiHash;Files=$files;IniChangeOnly=(($insert.Trim() -replace '\r?\n',';')+$(if($NativeSpeech){';Localization.NativePolishSpeech=1'}else{''})+$(if($UiNarrator){';Localization.UiNarratorEnabled=1'}else{''})+$(if($GlovesOnly){';Render.ShowControllerArms=0 (was1)'}else{''}));
  PhysicalAcceptance='PENDING';NativeSpeechEnabled=[bool]$NativeSpeech;TextureBridgeEnabled=[bool]$TextureBridge;
  GlovesOnlyEnabled=[bool]$GlovesOnly;UiNarratorEnabled=[bool]$UiNarrator;ModalUiProbeEnabled=[bool]$ModalUiProbe;
  WorldCinemaEnabled=[bool]$WorldCinema;
  Notes='Exact four-file core lease; optional owned ShowControllerArms1->0 and UI narrator; independent asset/WAV leases separate'} |
    ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $out 'test-manifest.json') -Encoding UTF8
Write-Output "OFFLINE READY: $out"
Write-Output 'No installed file changed; no game launched; fresh user start required'
