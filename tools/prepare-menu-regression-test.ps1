[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$SourceBundle,
    [Parameter(Mandatory)][string]$OutputDir,
    [Parameter(Mandatory)][string]$AcceptedProfilePath
)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$output=[IO.Path]::GetFullPath($OutputDir)
if(-not $output.StartsWith($repo.TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase)){
    throw 'Offline output must be inside this worktree'
}
if(Test-Path -LiteralPath $output){throw 'Refusing to overwrite candidate'}
$source=(Resolve-Path -LiteralPath $SourceBundle).Path
$plan=Get-Content -Raw (Join-Path $source 'change-plan.json') | ConvertFrom-Json
if($plan.profileSha256 -cne (Get-FileHash -Algorithm SHA256 $AcceptedProfilePath).Hash){throw 'Restore profile identity changed'}
$dll=Join-Path $repo 'spikes/view_matrix/d3d9.dll'
$manifest=Join-Path $repo 'spikes/view_matrix/d3d9.build.json'
$build=Get-Content -Raw $manifest | ConvertFrom-Json
$hash=(Get-FileHash -Algorithm SHA256 $dll).Hash
if($hash -cne $build.sha256 -or $build.architecture -ne 'x86 PE32' -or $build.optimization -ne '/O2'){
    throw 'Build provenance check failed'
}
New-Item -ItemType Directory -Path $output | Out-Null
& (Join-Path $repo 'tools/title/prepare_context_cursor_ini.ps1') -InputIni (Join-Path $source 'SingularityVR.ini') -OutputIni (Join-Path $output 'SingularityVR.ini') -RightEyeMirror -OpaqueXrCapture
$iniPath=Join-Path $output 'SingularityVR.ini'
$text=[IO.File]::ReadAllText($iniPath)
if($text -match '(?im)^\s*KnifeMeleePoseProbe\s*='){throw 'Unexpected prior knife probe; inspect source bundle'}
if([regex]::Matches($text,'(?m)^\[Diagnostic\]\r?$').Count -ne 1){throw 'Expected exactly one Diagnostic section'}
$nl=if($text.Contains("`r`n")){"`r`n"}else{"`n"}
$text=$text.Replace('[Diagnostic]',"[Diagnostic]${nl}KnifeMeleePoseProbe=1")
[IO.File]::WriteAllText($iniPath,$text,[Text.UTF8Encoding]::new($false))
$profile=Get-Content -Raw $AcceptedProfilePath | ConvertFrom-Json
foreach($key in @('Render/NativeMuzzleParticleStereo','Render/XrCaptureOpaqueAlpha','Diagnostic/KnifeMeleePoseProbe')){
    if(@($plan.ini | Where-Object key -eq $key).Count){throw "Source already plans $key; inspect before reusing"}
    $before=$profile.ini.PSObject.Properties[$key]
    $plan.ini+= [pscustomobject]@{key=$key;before=$(if($before){$before.Value}else{$null});after='1';reason='Preserve accepted VFX / isolated opaque XR capture / bounded read-only knife pose evidence'}
}
$entry=@($plan.files | Where-Object path -eq 'Binaries/d3d9.dll')
if($entry.Count -ne 1){throw 'Expected one DLL change'}
$entry[0].after=$hash
$entry[0].reason='Menu and health changes retained; opaque XR capture experiment and read-only knife pose probe'
$plan.reason='Correct missing accepted VFX flag; verify opaque VR screenshots and investigate right-hand native melee motion'
$plan | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $output 'change-plan.json') -Encoding UTF8
Copy-Item -LiteralPath $dll,$manifest -Destination $output
Write-Output "Offline candidate prepared; no installation or launch: $output"
Get-FileHash -Algorithm SHA256 (Join-Path $output 'd3d9.dll'),$iniPath
