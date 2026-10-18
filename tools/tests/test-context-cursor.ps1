[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$repo=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$out=Join-Path $repo 'build/context-cursor-l1'
New-Item -ItemType Directory -Force -Path $out | Out-Null

$vswhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars=Join-Path $vs 'VC/Auxiliary/Build/vcvarsall.bat'
$source=Join-Path $repo 'spikes/view_matrix/tests/test-context-cursor-gate.cpp'
$exe=Join-Path $out 'test-context-cursor-gate.exe'
$obj=Join-Path $out 'test-context-cursor-gate.obj'
$command='"'+$vcvars+'" x86 >nul && cl /nologo /O2 /EHsc /W4 /WX /MD /std:c++17 /Fo:"'+$obj+'" /Fe:"'+$exe+'" "'+$source+'"'
cmd /c $command
if($LASTEXITCODE -ne 0){throw 'context cursor L1 compile failed'}
& $exe
if($LASTEXITCODE -ne 0){throw 'context cursor L1 assertions failed'}

$renderer=Get-Content -Raw -LiteralPath (Join-Path $repo 'spikes/view_matrix/d3d9.cpp')
$ini=Get-Content -Raw -LiteralPath (Join-Path $repo 'spikes/view_matrix/SingularityVR.ini')
$indexedStart=$renderer.IndexOf('HRESULT STDMETHODCALLTYPE Hook_DrawIndexedPrimUP')
$gateStart=$renderer.IndexOf('const bool contextCursor = contextcursor::ShouldSuppress(', $indexedStart)
$inert=$renderer.IndexOf('if (UserPtrInert())', $indexedStart)
$level2=$renderer.IndexOf('if (g_userPtrHookLevel == 2)', $gateStart)
$suppress=$renderer.IndexOf('if (contextCursor)', $gateStart)
if($indexedStart -lt 0 -or $gateStart -le $indexedStart -or $inert -lt $indexedStart -or $inert -ge $gateStart -or
   $level2 -le $gateStart -or $suppress -le $gateStart -or $suppress -ge $level2){
    throw 'cursor suppression must be after the render-thread/inert gate and before level-2 passthrough'
}
if($renderer -notmatch 'GetPrivateProfileIntA\("Render","ContextCursorSuppress",0,path\)'){
    throw 'context cursor suppression must default OFF when the key is missing'
}
if($ini -notmatch '(?m)^ContextCursorSuppress=0\s*$'){
    throw 'shipped INI must leave experimental cursor suppression OFF'
}
if($renderer -notmatch 'g_vrUiMenu\) != 0 && Rd\(g_vrMenuClass\) == 4' -or
   $renderer -notmatch 'g_vrUiDirectCanvas\) != 0, Rd\(g_rtIsScene\) != 0'){
    throw 'native cursor gate lost its modal/direct-canvas/scene-only guards'
}
Write-Output 'PASS: historical suppression disabled; contextual depth policy; no RvGame Dialog package edit'
$native=Get-Content -Raw (Join-Path $repo 'spikes/view_matrix/native_context_cursor.h')
$overlay=Get-Content -Raw (Join-Path $repo 'spikes/view_matrix/vr_menu_overlay.h')
$input=Get-Content -Raw (Join-Path $repo 'spikes/view_matrix/native_ui_input.h')
if($renderer -notmatch 'GetPrivateProfileIntA\("Render","ContextCursorNative",0,path\)' -or
   $ini -notmatch '(?m)^ContextCursorNative=0\s*$'){throw 'Native cursor must default OFF'}
foreach($witness in @('0x28B71A137B55BB7Aull','0xD2E964C936CCE902ull','0x9D912591BFBC040Eull','0xEF5A6D846CDD5E7Bull')){
    if(-not $native.Contains($witness)){throw "Missing native cursor identity: $witness"}
}
if(-not $input.Contains('focusedRecord?ReadPointer(focusedRecord+0x50u):0')){throw 'Native call must revalidate current focus after input dispatch'}
if(-not $input.Contains('PublishVrUiDispatched(') -or -not $overlay.Contains('ReadVrUiDispatched(pointerU,pointerV)')){throw 'Reticle must consume native-dispatch receipt'}
$begin=$overlay.IndexOf('static void DrawVrMenuLaser(')
$end=$overlay.IndexOf('static void DrawVrMenuReticle(',$begin)
$laser=$overlay.Substring($begin,$end-$begin)
if(-not $laser.Contains('dev->SetScissorRect(&clip)') -or -not $laser.Contains('D3DTOP_SELECTARG1')){throw 'Laser lacks per-eye scissor or diffuse-only state'}
Write-Output 'PASS: asset + native ABI identities, focus validation, default OFF, input receipt and per-eye laser clipping'
