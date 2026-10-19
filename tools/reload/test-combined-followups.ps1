[CmdletBinding()]
param([Parameter(Mandatory)][string]$OriginalPackage)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$out=Join-Path $repo 'build/combined-followups'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$vswhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars=Join-Path $vs 'VC/Auxiliary/Build/vcvarsall.bat'
foreach($test in @('test-combined-followups','test-owned-inventory-scope')){
 $source=Join-Path $repo "spikes/view_matrix/tests/$test.cpp"
 $exe=Join-Path $out "$test.exe";$obj=Join-Path $out "$test.obj"
 cmd /c "`"$vcvars`" x86 >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /O2 /MD /Fe:`"$exe`" /Fo:`"$obj`" `"$source`""
 if($LASTEXITCODE -ne 0){throw "Compile failed: $test"}
 & $exe $OriginalPackage
 if($LASTEXITCODE -ne 0){throw "Test failed: $test"}
}
foreach($test in @('test_combined_followups.py','test_native_action_hands.py','test_inventory_registration.py')){
 python (Join-Path $PSScriptRoot $test)
 if($LASTEXITCODE -ne 0){throw "Python test failed: $test"}
}
& (Join-Path $repo 'tools/tests/test-combined-followups-prep.ps1')
& (Join-Path $repo 'tools/tests/test-followup-lease.ps1')
Write-Output 'Combined followups L1 PASS; no Steam/game process access, no L3/L4 claim'
