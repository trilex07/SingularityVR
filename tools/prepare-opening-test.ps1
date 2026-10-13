[CmdletBinding()]
param([Parameter(Mandatory)][string]$OutputDirectory,
      [Parameter(Mandatory)][string]$VrDll,
      [Parameter(Mandatory)][string]$InjectorDll,
      [Parameter(Mandatory)][string]$AudioPackage,
      [switch]$FrontendHandoff,
      [switch]$CinemaMenuPair,
      [switch]$HandAnchorProbe,
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
$candidateIni=$text.Insert($position,$insert)
if($candidateIni.Remove($position,$insert.Length) -cne $text){throw 'INI changed outside owned experiment keys'}
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
  FrozenUiSha256=$uiHash;Files=$files;IniChangeOnly=($insert.Trim() -replace '\r?\n',';');
  PhysicalAcceptance='PENDING';Notes='Native audio and hand candidate; FPS attribution, not proven residual-FPS fix'} |
    ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $out 'test-manifest.json') -Encoding UTF8
Write-Output "OFFLINE READY: $out"
Write-Output 'No installed file changed; no game launched; fresh user start required'
