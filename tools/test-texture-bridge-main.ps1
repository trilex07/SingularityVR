param([Parameter(Mandatory)][string]$OutputDirectory)
# Offline hidden SDK-device test. No DllMain, game, Steam or OpenXR session calls.
$ErrorActionPreference='Stop'
$repo=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(-not $out.StartsWith($repo+'\deployment\hands-dubbing\',[StringComparison]::OrdinalIgnoreCase)){throw 'Private output required'}
New-Item -ItemType Directory -Path $out -Force | Out-Null
. (Join-Path $PSScriptRoot 'Resolve-OpenXrSdk.ps1');$xr=Get-OpenXrSdkPaths -RepoRoot $repo
$vs=& "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vc=Join-Path $vs 'VC/Auxiliary/Build/vcvarsall.bat';$mh=Join-Path $repo 'third_party/minhook'
$sources=@('spikes/view_matrix/tests/test-texture-bridge-main.cpp','spikes/texture_cache/tests/bridge-main-fixture.cpp','spikes/view_matrix/polish_localization.cpp')|ForEach-Object {'"'+(Join-Path $repo $_)+'"'}
$sources+=@('buffer.c','hook.c','trampoline.c','hde/hde32.c')|ForEach-Object {'"'+(Join-Path $mh ('src/'+$_))+'"'}
Push-Location $out
try{
 & cmd /c ("`"$vc`" x86 >nul && cl /nologo /O2 /EHsc /W3 /MD /std:c++17 /I`"$($xr.Inc)`" /I`"$mh/include`" /Fe:test-main.exe "+($sources -join ' ')+" /link /LIBPATH:`"$($xr.LibDir)`" openxr_loader.lib dxgi.lib user32.lib shell32.lib")
 if($LASTEXITCODE -ne 0){throw 'Actual MAIN integration test compile failed'}
 Copy-Item -LiteralPath (Join-Path $xr.BinDir 'openxr_loader.dll') -Destination (Join-Path $out 'openxr_loader.dll')
 & ./test-main.exe
 if($LASTEXITCODE -ne 0){throw 'Actual MAIN integration test failed'}
}finally{Pop-Location}
