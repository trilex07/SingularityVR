param([Parameter(Mandatory=$true)][string]$Asset)
$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/../..").Path
$out=Join-Path $root 'build/gloves/tmd-runtime-regressions'
New-Item -ItemType Directory -Force $out | Out-Null
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vc=Join-Path $vs 'VC/Auxiliary/Build/vcvarsall.bat'
$tests=@('test-tmd-aim','test-tmd-native-adapter','test-weapon-holster','test-native-glove-cuffs',
    'test-hands-only','test-vr-transition-sim','test-centurion-reload','test-next-weapon-reload',
    'test-measured-aim','test-locker-vr-loadout-preview','test-locker-vr-loadout-policy',
    'test-locker-vr-loadout-layout','test-tmd-short-gauntlet-d3d9')
Push-Location $out
try {
    foreach($test in $tests){
        $src=Join-Path $root "spikes/view_matrix/tests/$test.cpp"
        $args='';$libs=''
        if($test -eq 'test-tmd-short-gauntlet-d3d9'){$args="`"$Asset`"";$libs='/link d3d9.lib user32.lib'}
        cmd /c "`"$vc`" x86 >nul && cl /nologo /O2 /EHsc /std:c++17 `"$src`" /Fe:$test.exe $libs && $test.exe $args"
        if($LASTEXITCODE -ne 0){throw "Failed $test"}
    }
    foreach($test in @('tools/fx/test_tmd_aim_wiring.py',
        'spikes/view_matrix/tests/test-locker-vr-loadout-preview-paths.py')){
        py -3 (Join-Path $root $test)
        if($LASTEXITCODE -ne 0){throw "Failed $test"}
    }
    & (Join-Path $root 'spikes/view_matrix/tests/run-test-oled.ps1')
} finally {Pop-Location}
