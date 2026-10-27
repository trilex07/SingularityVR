$ErrorActionPreference='Stop'
$root=(Get-Location).Path
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vc=Join-Path $vs 'VC/Auxiliary/Build/vcvarsall.bat'
$out=Join-Path $root 'build/integration-results'
foreach($test in @('test-hand-anchor-probe','test-hand-anchor-math')) {
 $src=Join-Path $root "spikes/view_matrix/tests/$test.cpp"
 cmd /c "`"$vc`" x86 >nul && cl /nologo /O2 /EHsc /std:c++17 `"$src`" /Fo:`"$out/$test.obj`" /Fe:`"$out/$test.exe`" && `"$out/$test.exe`""
 if($LASTEXITCODE -ne 0){throw $test}
}
python tools/tests/test-hand-anchor-log.py
if($LASTEXITCODE -ne 0){throw 'parser failed'}
