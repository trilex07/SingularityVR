$ErrorActionPreference = 'Stop'
$root = Resolve-Path (Join-Path $PSScriptRoot '..')
$source = Get-Content -Raw -LiteralPath (Join-Path $root 'd3d9.cpp')
$overlay = Get-Content -Raw -LiteralPath (Join-Path $root 'vr_menu_overlay.h')
if ($source -match 'Log\("independent hands:') { throw 'per-frame independent-hand disk logging returned' }
if ($source -notmatch 'inline bool ArmsAnimActive\(\) \{ return Rd\(g_armsAnimActiveCached\)') { throw 'draw hooks still evaluate 64-bit animation deadlines' }
if ($source -notmatch 'Hook_SetTexture' -or $source -notmatch 'IsProjectedShadowMap') { throw 'sampled shadow projection path is absent' }
if ($source -match 'if \(Rd\(g_vrUiMenu\)\) return StereoPair') { throw 'menu still bypasses the canonical HUD path' }
if ($overlay -notmatch 'VrUiProjectEye\(eye,\s*g_vrUiHit\.world') { throw 'hit marker does not use the OpenXR eye projector' }
$ini = Get-Content -Raw -LiteralPath (Join-Path $root 'SingularityVR.ini')
if ($ini -match '(?m)^RenderEvidence=1$' -or $ini -match '(?m)^CameraTranslationEvidence=1$') { throw 'expensive evidence mode is enabled in shipping ini' }
Write-Host 'PASS: no frame-rate disk log; one menu projector; sampled-shadow suppression; evidence modes off'
if ($overlay -notmatch 'D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE' -or
    $overlay -notmatch 'D3DTSS_TEXCOORDINDEX, 0' -or
    $overlay -notmatch 'D3DTSS_COLOROP, D3DTOP_DISABLE') { throw 'pause fixed-function UV contract incomplete' }
if ($source -notmatch 'SaveBackbufferCapture\(dev, true\)' -or
    $overlay -match 'captures\+\+ < 3\) InterlockedExchange\(&g_captureRequested') { throw 'pause diagnostic still captures restored RT0' }
Write-Host 'PASS: pause UV contract explicit; bounded capture reads actual XR backbuffer'
foreach ($key in @('ShadowWorldReceiverFix','ShadowForegroundClearFix','ShadowFirstPersonReceiverFix','InputEnumerationCache')) {
    if ($source -notmatch ('GetPrivateProfileIntA\("(?:Render|Input)","'+$key+'",1,path\)') -or
        $ini -notmatch ('(?m)^'+$key+'=1\r?$')) { throw "Accepted fix lost persistent ON default: $key" }
}
Write-Host 'PASS: accepted shadow/input defaults enabled without temporary flags or old-INI replacement'
