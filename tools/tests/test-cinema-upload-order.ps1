param([string]$SourceRef='')
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$out=Join-Path $root 'build/cinema-upload-order'
New-Item -ItemType Directory -Force $out | Out-Null
$src=Join-Path $out 'actual-hook-prefix.cpp'
if($SourceRef){python "$PSScriptRoot/generate-cinema-upload-order-fixture.py" $src $SourceRef}else{python "$PSScriptRoot/generate-cinema-upload-order-fixture.py" $src}
if($LASTEXITCODE){throw 'fixture generation failed'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vc=Join-Path $vs 'VC/Auxiliary/Build/vcvarsall.bat'
foreach($arch in @('x86','x64')){
 $exe=Join-Path $out "upload-$arch.exe";$obj=Join-Path $out "upload-$arch.obj"
 cmd /c "`"$vc`" $arch >nul && cl /nologo /EHsc /std:c++17 /O2 /W4 /MD /Fe:`"$exe`" /Fo:`"$obj`" `"$src`""
 if($LASTEXITCODE){throw 'compile upload order'}
 & $exe;if($LASTEXITCODE){throw 'actual upload-order regression failed'}
}
