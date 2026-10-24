$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/..").Path
. (Join-Path $root 'tools\Resolve-OpenXrSdk.ps1')
$xr=Get-OpenXrSdkPaths -RepoRoot $root
$out=Join-Path $root 'build/fixtures'
New-Item -ItemType Directory -Force $out | Out-Null
$vswhere='C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vc=Join-Path $vs 'VC/Auxiliary/Build/vcvarsall.bat'
$tests=@{
    'test-tmd-haptics'             = ''
    'test-ui-remaster-d3d9'        = '/link d3d9.lib user32.lib'
    'test-ui-remaster-runtime'     = ''
    'test-ui-remaster-switch-combos' = ''
}
Push-Location $out
try {
    $n=0
    foreach($name in $tests.Keys){
        $src=Join-Path $root "spikes/view_matrix/tests/$name.cpp"
        if(-not (Test-Path $src)){ Write-Output "$name MISSING (predecessor branch only)"; continue }
        cmd /c "`"$vc`" x86 >nul && cl /nologo /O2 /EHsc /std:c++17 /I`"$($xr.Inc)`" /I`"C:\Users\Trilex\vr-mods\SingularityVR-m02-m09\spikes\view_matrix`" `"$src`" /Fe:$name.exe $($tests[$name])" > $null 2>&1
        if($LASTEXITCODE -ne 0){ throw "compile failed: $name" }
        Write-Output "=== $name ==="
        cmd /c ".\$name.exe"
        if($LASTEXITCODE -ne 0){ throw "run failed: $name" }
        $n++
    }
    Write-Output "fixtures run: $n PASS"
} finally { Pop-Location }
