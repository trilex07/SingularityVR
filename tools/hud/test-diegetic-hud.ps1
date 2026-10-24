param([switch]$SkipGpu)
$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/../..").Path
$out=Join-Path $root 'build/fixtures'
New-Item -ItemType Directory -Force $out | Out-Null
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vc=Join-Path $vs 'VC/Auxiliary/Build/vcvarsall.bat'
$tests=@('test-diegetic-centurion','test-diegetic-projection')
if(!$SkipGpu){$tests+='test-diegetic-d3d9'}
Push-Location $out
try {
    foreach($test in $tests){
        $libs=''
        if($test -eq 'test-diegetic-d3d9'){$libs='/link d3d9.lib user32.lib ole32.lib windowscodecs.lib'}
        $src=Join-Path $root "spikes/view_matrix/tests/$test.cpp"
        cmd /c "`"$vc`" x86 >nul && cl /nologo /O2 /EHsc /std:c++17 `"$src`" /Fe:$test.exe $libs && $test.exe"
        if($LASTEXITCODE -ne 0){throw "Failed $test"}
    }
} finally {Pop-Location}
