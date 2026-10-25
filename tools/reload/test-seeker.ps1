param([string]$IndexAsset)
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vc=Join-Path $vs 'VC/Auxiliary/Build/vcvarsall.bat'
$out=Join-Path $root 'build/seeker-private/tests'
New-Item -ItemType Directory -Force $out | Out-Null
# Use the real production name registry: synthetic offsets must not hide
# unregistered names (the PR60 ownership gate defect).
$source=Get-Content (Join-Path $root 'spikes/view_matrix/d3d9.cpp') -Raw
$start=$source.IndexOf('g_wantNames[]')
if($start -lt 0){throw 'Missing production name registry'}
$end=$source.IndexOf('const int kWantNameCount',$start)
$names=[regex]::Matches($source.Substring($start,$end-$start),'\{\s*"([^"]+)"\s*,\s*-1\s*\}')
$literal=($names | ForEach-Object { '"'+$_.Groups[1].Value+'"' }) -join ','
('static const char* productionNames[]={' + $literal + '};') | Set-Content (Join-Path $out 'seeker_registered_names.h') -Encoding ascii
Push-Location $out
try {
    foreach($name in @('test-seeker','test-seeker-native-adapter','test-seeker-projectile-adapter')){
        $src=Join-Path $root "spikes/view_matrix/tests/$name.cpp"
        cmd /c "`"$vc`" x86 >nul && cl /nologo /O2 /EHsc /std:c++17 /I. `"$src`" /Fe:$name.exe && $name.exe"
        if($LASTEXITCODE -ne 0){throw "Failed $name"}
    }
    if($IndexAsset){
        $src=Join-Path $root 'spikes/view_matrix/tests/test-seeker-index-d3d9.cpp'
        $asset=(Resolve-Path (Join-Path $root $IndexAsset)).Path
        cmd /c "`"$vc`" x86 >nul && cl /nologo /O2 /EHsc /std:c++17 /I. `"$src`" /Fe:test-seeker-index-d3d9.exe /link d3d9.lib user32.lib && test-seeker-index-d3d9.exe `"$asset`""
        if($LASTEXITCODE -ne 0){throw 'Failed Seeker D3D9/Ex IB identity test'}
    }
}finally{Pop-Location}
