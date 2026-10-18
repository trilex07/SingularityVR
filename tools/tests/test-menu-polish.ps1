param()
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$out=Join-Path $repo 'build/menu-polish-tests'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars=Join-Path $vs 'VC/Auxiliary/Build/vcvarsall.bat'
$source=Join-Path $repo 'spikes/view_matrix/tests/test-menu-polish.cpp'
$exe=Join-Path $out 'test-menu-polish.exe'
cmd /c "`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /MD /Fe:`"$exe`" /Fo:`"$out/test-menu-polish.obj`" `"$source`""
if($LASTEXITCODE -ne 0){throw 'menu polish L1 compile failed'}
& $exe
if($LASTEXITCODE -ne 0){throw 'menu polish L1 assertions failed'}

$renderer=Get-Content -Raw -LiteralPath (Join-Path $repo 'spikes/view_matrix/d3d9.cpp')
$overlay=Get-Content -Raw -LiteralPath (Join-Path $repo 'spikes/view_matrix/vr_menu_overlay.h')
$input=Get-Content -Raw -LiteralPath (Join-Path $repo 'spikes/view_matrix/native_ui_input.h')
$policy=Get-Content -Raw -LiteralPath (Join-Path $repo 'spikes/view_matrix/modal_menu_policy.h')
$ini=Get-Content -Raw -LiteralPath (Join-Path $repo 'spikes/view_matrix/SingularityVR.ini')
if($renderer -notmatch 'const float screenU=uipointer::CanvasToEye\(u,g_vrMenuPanel\.plane\.inset\)' -or
   $renderer -notmatch 'const float screenV=uipointer::CanvasToEye\(v,g_vrMenuPanel\.plane\.inset\)' -or
   $renderer -match 'const float localU = dup \?'){
    throw 'VR Settings hit-test must use the inset head-plane UV, not a second SBS split'
}
if($ini -notmatch '(?m)^VrPromptTokens=1\s*$'){
    throw 'New installations must enable exact VR action-token labels'
}
if($ini -notmatch '(?m)^FrontMenuScalePct=72\s*$'){throw 'frontend menu scale is not 72 percent'}
if($renderer -notmatch 'handhud::Visible\(Rd\(g_healthPlate\)' -or
   $renderer -notmatch 'handhud::Visible\(Rd\(g_ammoPlate\)' -or
   $renderer -notmatch 'handhud::Visible\(Rd\(g_wristHealth\)'){throw 'wrist, health-plate, and ammo HUD gates must use gameplay policy'}
if($overlay -notmatch 'modalmenu::CaptureExactDialog' -or
   $overlay -notmatch 'g_vrModalDialogSurface'){throw 'dialog capture is not bound to its exact reflected surface'}
if($renderer -notmatch 'modalmenu::SuppressParentSlots' -or
   $renderer -notmatch 'g_vrModalParentStageBound'){throw 'parent Slots suppression is not source-scoped'}
if($renderer -notmatch 'modalmenu::DirectDialogCanSuppressParent' -or
   $input -notmatch 'ExactDirectDialogPath' -or
   $input -notmatch 'RenderTextureMode=0' -or
   $policy -notmatch 'DirectDialogCanSuppressParent'){throw 'direct Dialog fallback lacks reflected null-target/mode proof'}
$syncStart=$overlay.IndexOf('void SyncVrMenuCaptureState()')
$captureStart=$overlay.IndexOf('void CaptureVrMenuTarget(IDirect3DDevice9* dev)',$syncStart)
if($syncStart -lt 0 -or $captureStart -le $syncStart){throw 'menu capture state transition was not found'}
$syncBody=$overlay.Substring($syncStart,$captureStart-$syncStart)
if($syncBody.IndexOf('PreserveModalParentSources();') -lt 0 -or
   $syncBody.IndexOf('PreserveModalParentSources();') -gt $syncBody.IndexOf('const auto token=VrMenuStateToken();')){
 throw 'exact parent Slots source must be retained before token transition clears it'
}
if($overlay -notmatch 'DrawVrMenuReticle\(dev,d,endpoint' -or
   $overlay -notmatch 'DrawVrMenuLaser\(dev,d,endpoint' -or
   $overlay -notmatch 'PointAtCanvasUv\(\s*g_vrMenuPanel.plane'){throw 'laser and reticle do not share the normalized input point'}
if($renderer -notmatch 'InterlockedExchange64\(&g_vrUiPairPacked' -or
   $renderer -notmatch 'InterlockedCompareExchange64\(&g_vrUiPairPacked'){throw 'input and overlay coordinates are not published as one atomic UV pair'}
if($input -notmatch 'pin the native mouse at the last VR'){throw 'missed rays can fall through to the desktop cursor'}
if($renderer -notmatch 'InterlockedCompareExchange\(&g_wwSeen\[k\], curV, 0\)' -or
   $renderer -match 'InterlockedCompareExchange\(&g_wwSeen\[k\], 0, curV\)'){
    throw 'weapon wheel seen-set must atomically write the current weapon into an empty slot'
}
Write-Host 'PASS: projective GFx/pointer agreement, shared laser endpoint, modal source retention and HUD policies'
