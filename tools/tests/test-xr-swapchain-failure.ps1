$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
. (Join-Path $repo 'tools/Resolve-OpenXrSdk.ps1')
$xr = Get-OpenXrSdkPaths -RepoRoot $repo

$fixture = Join-Path $repo 'spikes/view_matrix/tests/test-xr-swapchain-failure.cpp'
$source = Get-Content (Join-Path $repo 'spikes/view_matrix/d3d9.cpp') -Raw
# Compile the production bodies verbatim against controlled OpenXR responses.
$start = $source.IndexOf('static void ClearStereoSwapchains()')
$end = $source.IndexOf('// ---- did the engine ACCEPT the resolution', $start)
if ($start -lt 0 -or $end -le $start) { throw 'Swapchain source boundaries moved' }
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$vsPath = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars = Join-Path $vsPath 'VC/Auxiliary/Build/vcvarsall.bat'
$scratch = Join-Path ([IO.Path]::GetTempPath()) ("singularity-xr-swapchain-" + [guid]::NewGuid())
New-Item -ItemType Directory -Path $scratch | Out-Null
Push-Location -LiteralPath $scratch
try {
    $source.Substring($start, $end - $start) | Set-Content xr-swapchain-under-test.inl -Encoding UTF8
    @"
@call "$vcvars" x86 >nul
@if errorlevel 1 exit /b 1
@cl /nologo /std:c++17 /EHsc /O2 /I"$scratch" /I"$($xr.Inc)" "$fixture" /Fe:swapchain-test.exe
@exit /b %ERRORLEVEL%
"@ | Set-Content build.cmd -Encoding ASCII
    & ./build.cmd
    if ($LASTEXITCODE) { throw 'Swapchain fixture build failed' }
    & ./swapchain-test.exe
    if ($LASTEXITCODE) { throw 'Swapchain fixture assertions failed' }
} finally {
    Pop-Location
    Get-ChildItem -LiteralPath $scratch -File | ForEach-Object { Remove-Item -LiteralPath $_.FullName }
    Remove-Item -LiteralPath $scratch
}
