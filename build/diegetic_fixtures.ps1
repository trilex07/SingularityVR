$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/..").Path
. (Join-Path $root 'tools\Resolve-OpenXrSdk.ps1')
$xr=Get-OpenXrSdkPaths -RepoRoot $root
$out=Join-Path $root 'build/fixtures'
New-Item -ItemType Directory -Force $out | Out-Null
$vswhere='C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vc=Join-Path $vs 'VC/Auxiliary/Build/vcvarsall.bat'
$tests=@('test-diegetic-centurion','test-tmd-haptics','test-ui-remaster-runtime','test-ui-remaster-switch-combos',
         'test-ui-remaster-d3d9','test-gameplay-text-probe','test-gameplay-text-safe-area',
         'test-hud-reseat','test-menu-hud-scale')
Push-Location $out
try {
    foreach($name in $tests){
        $src=Join-Path $root "spikes/view_matrix/tests/$name.cpp"
        if(-not (Test-Path $src)){ Write-Output "$name MISSING"; continue }
        $libs=''
        if($name -match 'd3d9'){$libs='/link d3d9.lib user32.lib'}
        cmd /c "`"$vc`" x86 >nul && cl /nologo /O2 /EHsc /std:c++17 /I`"$($xr.Inc)`" /I`"$root\spikes\view_matrix`" `"$src`" /Fe:$name.exe $libs" > $null 2>&1
        if($LASTEXITCODE -ne 0){ throw "compile failed: $name" }
        Write-Output "=== $name ==="
        cmd /c ".\$name.exe" | Select-Object -Last 3
        if($LASTEXITCODE -ne 0){ throw "run failed: $name" }
    }
} finally { Pop-Location }
