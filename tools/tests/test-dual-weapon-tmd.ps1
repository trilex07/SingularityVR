param([switch]$SkipOled)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vc = Join-Path $vs 'VC/Auxiliary/Build/vcvarsall.bat'
$out = Join-Path $root 'build/dual-tmd-tests'
New-Item -ItemType Directory -Force $out | Out-Null
$tests = @(
    'spikes/view_matrix/tests/test-dual-weapon-tmd.cpp',
    'spikes/view_matrix/tests/test-support-grip-probe.cpp',
    'spikes/view_matrix/tests/test-dual-hand-pose-probe.cpp',
    'spikes/view_matrix/tests/test-dual-hand-action-pose.cpp',
    'spikes/view_matrix/tests/test-dual-tmd-weapon-pose.cpp',
    'spikes/view_matrix/tests/test-tmd-support-gauntlet.cpp',
    'spikes/view_matrix/tests/test-tmd-aim.cpp',
    'spikes/view_matrix/tests/test-tmd-native-adapter.cpp',
    'spikes/view_matrix/tests/test-weapon-holster.cpp',
    'spikes/view_matrix/tests/test-native-glove-cuffs.cpp',
    'spikes/view_matrix/tests/test-hands-only.cpp',
    'spikes/view_matrix/tests/test-vr-transition-sim.cpp',
    'spikes/view_matrix/tests/test-centurion-reload.cpp',
    'spikes/view_matrix/tests/test-next-weapon-reload.cpp',
    'spikes/view_matrix/tests/test-weapon-carry-ray.cpp',
    'spikes/view_matrix/tests/test-two-hand-spread.cpp',
    'spikes/view_matrix/tests/test-weapon-accuracy-policy.cpp',
    'spikes/view_matrix/tests/test-measured-aim.cpp',
    'spikes/view_matrix/tests/test-locker-vr-loadout-preview.cpp',
    'spikes/view_matrix/tests/test-locker-vr-loadout-policy.cpp',
    'spikes/view_matrix/tests/test-locker-vr-loadout-layout.cpp',
    'tools/reference-tests/hands/glove_isolate_test.cpp'
)
Push-Location $out
try {
    foreach($test in $tests) {
        $source = Join-Path $root $test
        $exe = Join-Path $out ([IO.Path]::GetFileNameWithoutExtension($test)+'.exe')
        cmd /c "`"$vc`" x86 >nul && cl /nologo /O2 /EHsc /std:c++17 `"$source`" /Fe:`"$exe`" && `"$exe`""
        if($LASTEXITCODE -ne 0){throw "Failed: $test"}
    }
    foreach($test in @('tools/fx/test_dual_weapon_tmd_wiring.py','tools/fx/test_tmd_aim_wiring.py',
                      'spikes/view_matrix/tests/test-locker-vr-loadout-preview-paths.py')) {
        python (Join-Path $root $test)
        if($LASTEXITCODE -ne 0){throw "Failed: $test"}
    }
    if(!$SkipOled){& (Join-Path $root 'spikes/view_matrix/tests/run-test-oled.ps1')}
} finally {Pop-Location}
