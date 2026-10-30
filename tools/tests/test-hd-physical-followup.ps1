param(
 [string]$Package=(Join-Path $PSScriptRoot '../../../kasimov_snv_e99/final_handoff'),
 [string]$Mesh='',
 [string]$ReloadAssets=''
)
$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/../..").Path
if(!$Mesh){$Mesh=Join-Path $root 'build/kasimov-hd/runtime/kasimov_hd.svks'}
$out=Join-Path $root 'build/hd-physical-followup'
New-Item -ItemType Directory -Force $out | Out-Null
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vc=Join-Path $vs 'VC/Auxiliary/Build/vcvarsall.bat'
foreach($test in @('test-hd-physical-followup','test-weapon-carry-ray','test-next-weapon-runtime')){
 if($test -eq 'test-next-weapon-runtime' -and !$ReloadAssets){continue}
 $source=Join-Path $root "spikes/view_matrix/tests/$test.cpp"
 $arguments=if($test -eq 'test-hd-physical-followup'){"`"$Package/kasimov_reload.svwa`" `"$Mesh`""}elseif($test -eq 'test-next-weapon-runtime'){"`"$ReloadAssets`""}else{''}
 cmd /c "`"$vc`" x86 >nul && cl /nologo /O2 /EHsc /std:c++17 `"$source`" /Fo:`"$out/$test.obj`" /Fe:`"$out/$test.exe`" && `"$out/$test.exe`" $arguments"
 if($LASTEXITCODE){throw "Failed: $test"}
}
