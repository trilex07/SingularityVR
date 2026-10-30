param([string]$PrivateRoot='build/seeker-private')
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$private=Join-Path $repo $PrivateRoot
$out=Join-Path $private 'tests'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$vswhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars=Join-Path $vs 'VC\Auxiliary\Build\vcvarsall.bat'
foreach($name in @('test-seeker-rear-reload','test-next-weapon-animation','test-next-weapon-runtime','test-next-weapon-refill-visual','test-weapon-carry-ray')){
    $src=Join-Path $repo "spikes/view_matrix/tests/$name.cpp"
    $exe=Join-Path $out "$name.exe"
    $obj=Join-Path $out "$name.obj"
    cmd /c "`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /MD /Fe:`"$exe`" /Fo:`"$obj`" `"$src`""
    if($LASTEXITCODE -ne 0){throw "Compile failed: $name"}
    switch($name){
      'test-next-weapon-animation' {& $exe --seeker (Join-Path $private 'prepared-root')}
      'test-next-weapon-refill-visual' {& $exe (Join-Path $private 'prepared-root')}
      default {& $exe}
    }
    if($LASTEXITCODE -ne 0){throw "Fixture failed: $name"}
}
Write-Host 'OFFLINE_SEEKER_CPU_PASS'
