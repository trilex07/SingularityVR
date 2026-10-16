# SIDE-only private staging. Fixed output beneath this checkout's ignored build
# directory; no Steam targets, INI copy, launch, deployment or game restoration.
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$build=Join-Path $repo 'build\centurion-reload'
$source=Join-Path $repo 'spikes\view_matrix'
$manifest=Get-Content -LiteralPath (Join-Path $source 'd3d9.build.json') -Raw | ConvertFrom-Json
$dll=Join-Path $source 'd3d9.dll'
if($manifest.branch -notin @('feature/centurion-manual-reload','fix/centurion-grip-shoulder') -or
   $manifest.optimization -ne '/O2' -or $manifest.machine -ne '0x014C' -or
   $manifest.optionalHeaderMagic -ne '0x010B' -or $manifest.workingTreeStatus.Count -ne 0){
    throw 'Requires clean-source Centurion x86 PE32 /O2 provenance'
}
if((Get-FileHash -LiteralPath $dll -Algorithm SHA256).Hash -ne $manifest.sha256){throw 'DLL/build-manifest mismatch'}
& git -C $repo diff --quiet $manifest.commit -- spikes/view_matrix third_party tools/Resolve-OpenXrSdk.ps1
if($LASTEXITCODE -ne 0){throw 'Runtime source differs from the built commit; rebuild first'}
$output=Join-Path $build 'candidate'
$assets=Join-Path $output 'vr-assets\reload\centurion'
$resolvedOutput=[IO.Path]::GetFullPath($output)
$resolvedBuild=[IO.Path]::GetFullPath($build)+[IO.Path]::DirectorySeparatorChar
if(-not $resolvedOutput.StartsWith($resolvedBuild,[StringComparison]::OrdinalIgnoreCase)){
    throw 'Private output escaped this checkout build directory'
}
$expected=[ordered]@{
    'centurion_clip.svrk'='F21961145D6645EE93A8A2C73AE89380535D918352BABB1FD4F811C67422D89B'
    'centurion_clip.svrt'='17A3355AD6CC7E2F35A03883FDF4B314577FEE908773262C5BC199C006C6CD60'
}
# Verify every input before writing the private candidate; refuse stale or
# differently calibrated assets, rather than silently accepting new hashes.
foreach($name in $expected.Keys){
    $path=Join-Path (Join-Path $build 'native-clip') $name
    if((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $expected[$name]){
        throw "Private original asset hash mismatch: $name"
    }
}
New-Item -ItemType Directory -Force -Path $assets | Out-Null
Copy-Item -LiteralPath $dll -Destination (Join-Path $output 'd3d9.dll') -Force
Copy-Item -LiteralPath (Join-Path $source 'd3d9.build.json') -Destination (Join-Path $output 'd3d9.build.json') -Force
if((Get-FileHash -LiteralPath (Join-Path $output 'd3d9.dll') -Algorithm SHA256).Hash -ne $manifest.sha256){
    throw 'Private staged DLL mismatch'
}
$files=@()
foreach($name in $expected.Keys){
    $target=Join-Path $assets $name
    Copy-Item -LiteralPath (Join-Path (Join-Path $build 'native-clip') $name) -Destination $target -Force
    $hash=(Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash
    if($hash -ne $expected[$name]){throw "Private staged asset mismatch: $name"}
    $files+= [ordered]@{path="vr-assets/reload/centurion/$name";sha256=$hash;bytes=(Get-Item -LiteralPath $target).Length}
}
$receipt=[ordered]@{
    scope='PRIVATE SIDE candidate only; no INI or Steam writes'
    sourceCommit=$manifest.commit
    dllSha256=$manifest.sha256
    architecture=$manifest.architecture
    optimization=$manifest.optimization
    files=$files
    flagsDefaultOff=@('CenturionManualReload','CenturionNativeReloadTap','CenturionReloadProbe','CenturionClipPreview','CenturionReloadMarkers','ReloadControlLayout','CenturionCylinderGestures','CenturionNativeReloadGuard')
    limitations='Native guard OFF/observe/guard experiment requires MAIN runtime proof; original cylinder animation remains, ammo UE3-owned; albedo-only preview; L3/L4 UNKNOWN'
}
$receipt | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $output 'centurion-preview.manifest.json') -Encoding UTF8
$receipt | ConvertTo-Json -Depth 6
