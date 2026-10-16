param()
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$out=Join-Path $repo 'build/menu-holster-polish'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars=Join-Path $vs 'VC/Auxiliary/Build/vcvarsall.bat'
foreach($name in @('test-menu-hud-scale','test-holster-marker','test-weapon-holster','test-pause-menu-space','test-menu-submit-policy','test-hip-knife')){
    $source=Join-Path $repo "spikes/view_matrix/tests/$name.cpp"
    $exe=Join-Path $out "$name.exe"
    cmd /c "`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /MD /Fe:`"$exe`" /Fo:`"$out/$name.obj`" `"$source`""
    if($LASTEXITCODE -ne 0){throw "compile failed: $name"}
    if($name -eq 'test-holster-marker'){& $exe (Join-Path $out 'holster-preview.svg')}else{& $exe}
    if($LASTEXITCODE -ne 0){throw "test failed: $name"}
}
