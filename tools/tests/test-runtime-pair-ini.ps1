param([Parameter(Mandatory)][string]$OutputDirectory)
Set-StrictMode -Version Latest;$ErrorActionPreference='Stop'
$repo=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(-not $out.StartsWith($repo+'\deployment\',[StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $out)){throw 'Fresh private fixture required'}
$steam=Join-Path $out 'fake-steam';$bin=Join-Path $steam 'steamapps/common/Singularity/Binaries'
New-Item -ItemType Directory -Path $bin | Out-Null
$real='C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries'
$names=@('d3d9.dll','dinput8.dll','SingularityVR.ini');$baselines=@{}
foreach($name in $names){Copy-Item -LiteralPath (Join-Path $real $name) -Destination (Join-Path $bin $name);$baselines[$name]=(Get-FileHash -LiteralPath (Join-Path $bin $name)).Hash}
$protected=Join-Path $bin 'intro-and-assets-untouched.bin'
[IO.File]::WriteAllBytes($protected,[byte[]](1,2,3,4));$protectedSha=(Get-FileHash -LiteralPath $protected).Hash
$runner=Join-Path $repo 'tools/deploy-singularityvr.ps1'
$artifact=Join-Path $repo 'deployment/hands-dubbing/recovery-ready/core'
& $runner -Action Install -RuntimeDllPairOnly -IncludeIni -SteamRoot $steam -ArtifactDir $artifact -RepoRoot $repo
$reportFile=Get-ChildItem -LiteralPath (Join-Path $repo 'deployment/install-reports') -Filter '*.json' -File | Sort-Object LastWriteTime -Descending | Select-Object -First 1
$report=Get-Content -LiteralPath $reportFile.FullName -Raw|ConvertFrom-Json
if(@($report.Installed).Count -ne 3 -or $report.TextureCount -ne 0 -or -not $report.IncludeIni){throw 'Wrong deployment scope'}
foreach($name in $names){if((Get-FileHash -LiteralPath (Join-Path $bin $name)).Hash -cne (Get-FileHash -LiteralPath (Join-Path $artifact $name)).Hash){throw 'Candidate hash mismatch'}}
if((Get-FileHash -LiteralPath $protected).Hash -cne $protectedSha){throw 'Protected file changed'}
& $runner -Action Rollback -SteamRoot $steam -RepoRoot $repo -BackupPath $report.BackupPath
foreach($name in $names){if((Get-FileHash -LiteralPath (Join-Path $bin $name)).Hash -cne $baselines[$name]){throw 'Rollback mismatch'}}
Write-Output 'PASS: runtime pair+INI exact3 install/backup/rollback; no asset/intro/UI writes'
