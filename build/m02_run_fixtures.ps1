$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/..").Path
. (Join-Path $root 'tools\Resolve-OpenXrSdk.ps1')
$xr=Get-OpenXrSdkPaths -RepoRoot $root
Write-Output ("OpenXR SDK include: {0}" -f $xr.Inc)
$out=Join-Path $root 'build/m02-fixtures'
New-Item -ItemType Directory -Force $out | Out-Null
$vswhere='C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vc=Join-Path $vs 'VC/Auxiliary/Build/vcvarsall.bat'
$tests=@{
    'test-ui-remaster-d3d9'    = '/link d3d9.lib user32.lib'
    'test-ui-remaster-runtime' = ''
}
Push-Location $out
try {
    foreach($name in $tests.Keys){
        $src=Join-Path $root "spikes/view_matrix/tests/$name.cpp"
        cmd /c "`"$vc`" x86 >nul && cl /nologo /O2 /EHsc /std:c++17 /I`"$($xr.Inc)`" `"$src`" /Fe:$name.exe $($tests[$name])"
        Write-Output "$name build exit=$LASTEXITCODE"
        if($LASTEXITCODE -ne 0){ throw "compile failed: $name" }
    }
    Write-Output '=== run test-ui-remaster-runtime ==='
    cmd /c '.\test-ui-remaster-runtime.exe'
    Write-Output "runtime run exit=$LASTEXITCODE"
    Write-Output '=== run test-ui-remaster-d3d9 (HAL + EX) ==='
    cmd /c '.\test-ui-remaster-d3d9.exe'
    Write-Output "d3d9 run exit=$LASTEXITCODE"
} finally { Pop-Location }
