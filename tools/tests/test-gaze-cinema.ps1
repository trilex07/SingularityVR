$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$out=Join-Path $root 'build/gaze-cinema-tests'
New-Item -ItemType Directory -Force $out | Out-Null
$vs=& "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vc=Join-Path $vs 'VC/Auxiliary/Build/vcvarsall.bat'
foreach($arch in @('x86','x64')){foreach($name in @('gaze-interaction','cinema-stereo','damage-haptics')){
 $src=Join-Path $root "spikes/view_matrix/tests/test-$name.cpp"
 $exe=Join-Path $out "$name-$arch.exe";$obj=Join-Path $out "$name-$arch.obj"
 cmd /c "`"$vc`" $arch >nul && cl /nologo /EHsc /std:c++17 /O2 /W4 /MD /Fe:`"$exe`" /Fo:`"$obj`" `"$src`" /link d3d9.lib d3dcompiler.lib user32.lib"
 if($LASTEXITCODE -ne 0){throw "compile $name $arch"}
 & $exe;if($LASTEXITCODE -ne 0){throw "test $name $arch"}
}}
