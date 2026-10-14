[CmdletBinding()]
param([string]$OutputDirectory='', [switch]$DiagnosticTiming)
# Offline only. No Install switch, Steam launch, INI write or DDS copying.
$ErrorActionPreference='Stop'
$here=$PSScriptRoot
$root=(Resolve-Path (Join-Path $here '..\..')).Path
$out=if($OutputDirectory){[IO.Path]::GetFullPath($OutputDirectory)}else{Join-Path $root 'deployment\texture-cache\artifact'}
if(-not $out.StartsWith($root+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Output must stay inside this worktree'}
New-Item -ItemType Directory -Path $out -Force | Out-Null
$vsPath=& "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars=Join-Path $vsPath 'VC\Auxiliary\Build\vcvarsall.bat'
$mh=Join-Path $root 'third_party\minhook'
Push-Location $out
try {
    foreach($name in @('resource-native-cache','resource-native-cache-gpu','texture-hook-timing','texture-hook-timing-disabled')) {
        $test=Join-Path $here "tests\test-$name.cpp"
        & cmd /c "`"$vcvars`" x86 >nul && cl /nologo /O2 /EHsc /W3 /MD /std:c++17 /Fe:test-$name.exe `"$test`" /link user32.lib"
        if($LASTEXITCODE -ne 0){throw "Test compile failed: $name"}
        & (Join-Path $out "test-$name.exe")
        if($LASTEXITCODE -ne 0){throw "Test failed: $name"}
    }
    $source=Join-Path $here 'texture_injector.cpp'
    $timingFlag=if($DiagnosticTiming){' /DTEXCACHE_TIMING=1'}else{''}
    $analysis=& cmd /c "`"$vcvars`" x86 >nul && cl /nologo /EHsc /W3 /MD /std:c++17$timingFlag /analyze:only /I`"$mh\include`" /c `"$source`""
    $analysis | Write-Output
    if($LASTEXITCODE -ne 0){throw 'Static analysis compile failed'}
    if($analysis | Select-String 'warning C6067|warning C6063|warning C6064|warning C6066|warning C6270|warning C6271|warning C6272|warning C6273|warning C6328'){throw 'Format-string analysis failed'}
    $mhSources=@('buffer.c','hook.c','trampoline.c','hde\hde32.c') | ForEach-Object {'"'+(Join-Path $mh ('src\'+$_))+'"'}
    foreach($taskTest in @('texture-hook-forwarding','texture-bridge-gpu')){
        $forwardTest=Join-Path $here "tests\test-$taskTest.cpp"
        & cmd /c ("`"$vcvars`" x86 >nul && cl /nologo /O2 /EHsc /W3 /MD /std:c++17 /I`"$mh\include`" /Fe:test-$taskTest.exe `"$forwardTest`" " + ($mhSources -join ' ') + ' /link user32.lib')
        if($LASTEXITCODE -ne 0){throw "Actual hook test compile failed: $taskTest"}
        & (Join-Path $out "test-$taskTest.exe")
        if($LASTEXITCODE -ne 0){throw "Actual hook test failed: $taskTest"}
    }
    $definition=Join-Path $here 'dinput8.def'
    $compile="`"$vcvars`" x86 >nul && cl /nologo /LD /O2 /EHsc /W3 /MD /std:c++17$timingFlag /I`"$mh\include`" /Fe:dinput8.dll `"$source`" " + ($mhSources -join ' ') + " /link /DEBUG:NONE /MAP:dinput8.map /DEF:`"$definition`" user32.lib"
    & cmd /c $compile
    if($LASTEXITCODE -ne 0){throw 'Injector build failed'}
    $dll=Join-Path $out 'dinput8.dll'
    $bytes=[IO.File]::ReadAllBytes($dll)
    $pe=[BitConverter]::ToInt32($bytes,0x3c)
    if([BitConverter]::ToUInt32($bytes,$pe) -ne 0x4550 -or
       [BitConverter]::ToUInt16($bytes,$pe+4) -ne 0x14c -or
       [BitConverter]::ToUInt16($bytes,$pe+24) -ne 0x10b){throw 'Not x86 PE32'}
    $inputs=@('texture_injector.cpp','resource_native_cache.h','resource_positive_cache.h','remaster_scope.h','texture_hook_timing.h','texture_groups.h','targets_generated.h','dinput8.def','../view_matrix/texture_bridge_contract.h') | ForEach-Object {
        [ordered]@{name=$_;sha256=(Get-FileHash -LiteralPath (Join-Path $here $_)).Hash}
    }
    [ordered]@{builtAt=(Get-Date).ToString('o');branch=(& git -C $root branch --show-current).Trim();
        commit=(& git -C $root rev-parse HEAD).Trim();optimization='/O2';architecture='x86 PE32';
        sha256=(Get-FileHash -LiteralPath $dll).Hash;diagnosticTiming=[bool]$DiagnosticTiming;inputs=$inputs;
        trackedDiff=@(& git -C $root status --porcelain --untracked-files=no)} |
        ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $out 'dinput8.build.json') -Encoding UTF8
    Write-Output "Built verified x86 PE32/O2: $dll SHA $((Get-FileHash -LiteralPath $dll).Hash)"
} finally {Pop-Location}
