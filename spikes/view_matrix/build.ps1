# Builds the view-rotation detour shim as x86.
# Compiles MinHook's C sources alongside the C++ shim (no separate lib step needed).

param(
    [switch]$Install,
    [switch]$Performance,
    [string]$GameBinaries
)

$ErrorActionPreference = "Stop"
if ($Install) {
    throw 'Direct build -Install is disabled. Build without -Install, then use tools/deploy-singularityvr.ps1 with a reviewed accepted profile and exact change plan.'
}
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Resolve-Path (Join-Path $here "..\..")
. (Join-Path $root "tools\Resolve-OpenXrSdk.ps1")

$xr = Get-OpenXrSdkPaths -RepoRoot $root.Path
Write-Host ("OpenXR SDK: {0}" -f $xr.Sdk)
$mh = Join-Path $root "third_party\minhook"
$devGameBin = Get-SingularityGameBinaries -ExplicitPath $GameBinaries

$inc    = $xr.Inc
$libDir = $xr.LibDir
$binDir = $xr.BinDir
foreach ($p in @($inc, $libDir, $binDir, $mh)) { if (-not (Test-Path $p)) { throw "missing: $p" } }

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath = & $vswhere -latest -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvarsall.bat"

Push-Location $here
try {
    $mhSrc = @(
        "`"$mh\src\buffer.c`"",
        "`"$mh\src\hook.c`"",
        "`"$mh\src\trampoline.c`"",
        "`"$mh\src\hde\hde32.c`""
    ) -join " "

    # ---- static analysis pass on d3d9.cpp and polish_localization.cpp, before the real build ----
    $an = "`"$vcvars`" x86 >nul && cl /nologo /EHsc /W3 /MD /std:c++17 /analyze:only " +
          "/I`"$inc`" /I`"$mh\include`" /c d3d9.cpp polish_localization.cpp"
    $anOut = cmd /c $an
    $anOut | Write-Host
    $fatal = $anOut | Select-String -Pattern "warning C6067|warning C6063|warning C6064|warning C6066|warning C6270|warning C6271|warning C6272|warning C6273|warning C6328"
    if ($fatal) { throw "static analysis found a format-string defect - fix it before building" }

    # All production DLLs are optimized. Performance only enables the extra
    # instrumentation define; it no longer decides whether /O2 is used.
    $optimization = '/O2 '
    if ($Performance) { $optimization += '/DPERFORMANCE_BUILD_1 ' }
    # /Z7 keeps debug records in the object and avoids the shared vc140.pdb
    # writer, which is broken in some side-by-side VS installations (LNK1101).
    $cmd = "`"$vcvars`" x86 && cl /nologo /LD /EHsc /W3 /Z7 /MD /std:c++17 " + $optimization +
           "/I`"$inc`" /I`"$mh\include`" " +
           "/Fe:d3d9.dll d3d9.cpp polish_localization.cpp $mhSrc " +
           "/link /DEBUG:NONE /MAP:d3d9.map /DEF:d3d9.def /LIBPATH:`"$libDir`" openxr_loader.lib dxgi.lib user32.lib shell32.lib windowscodecs.lib ole32.lib"
    cmd /c $cmd
    if ($LASTEXITCODE -ne 0) { throw "build failed (exit $LASTEXITCODE)" }

    $builtDll = Join-Path $here 'd3d9.dll'
    $builtBytes = [System.IO.File]::ReadAllBytes($builtDll)
    if ($builtBytes.Length -lt 0x40 -or $builtBytes[0] -ne 0x4d -or $builtBytes[1] -ne 0x5a) {
        throw 'built output is not a PE image'
    }
    $peOffset = [BitConverter]::ToInt32($builtBytes, 0x3c)
    $machine = [BitConverter]::ToUInt16($builtBytes, $peOffset + 4)
    $optionalMagic = [BitConverter]::ToUInt16($builtBytes, $peOffset + 24)
    if ($machine -ne 0x014c -or $optionalMagic -ne 0x010b) {
        throw ('production DLL must be x86 PE32 (machine=0x{0:X4}, optional=0x{1:X4})' -f $machine, $optionalMagic)
    }
    $gitBranch = (& git -C $root branch --show-current).Trim()
    $gitCommit = (& git -C $root rev-parse HEAD).Trim()
    $gitStatus = @(& git -C $root status --short)
    $buildManifest = [ordered]@{
        builtAt = (Get-Date).ToString('o')
        repository = $root.Path
        branch = $gitBranch
        commit = $gitCommit
        workingTreeStatus = $gitStatus
        optimization = '/O2'
        performanceInstrumentation = [bool]$Performance
        architecture = 'x86 PE32'
        machine = ('0x{0:X4}' -f $machine)
        optionalHeaderMagic = ('0x{0:X4}' -f $optionalMagic)
        sha256 = (Get-FileHash -LiteralPath $builtDll -Algorithm SHA256).Hash.ToUpperInvariant()
    }
    $manifestPath = Join-Path $here 'd3d9.build.json'
    $buildManifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $manifestPath -Encoding UTF8
    Write-Host ""
    Write-Host "Built: $builtDll"
    Write-Host ("Build provenance: {0} (SHA-256 {1})" -f $manifestPath, $buildManifest.sha256)

} finally {
    Pop-Location
}
