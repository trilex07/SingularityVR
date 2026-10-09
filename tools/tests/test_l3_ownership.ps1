$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$root = Join-Path $repo '.test-l3-ownership-fixture'
if (Test-Path -LiteralPath $root) { Remove-Item -LiteralPath $root -Recurse -Force }
$gameBin = Join-Path $root 'steamapps\common\Singularity\Binaries'
$backup = Join-Path $root 'backup'
New-Item -ItemType Directory -Force -Path $gameBin | Out-Null
[IO.File]::WriteAllText((Join-Path $gameBin 'd3d9.dll'), 'before-dll')
[IO.File]::WriteAllText((Join-Path $gameBin 'SingularityVR.ini'), 'before-ini')
[IO.File]::WriteAllText((Join-Path $gameBin 'SingularityVfx.dll'), 'before-vfx')
$guard = Join-Path $repo 'tools\l3-steam-guard.ps1'
& pwsh -NoProfile -File $guard -Action Backup -SteamRoot $root -BackupDirectory $backup | Out-Null
[IO.File]::WriteAllText((Join-Path $gameBin 'd3d9.dll'), 'after-dll')
[IO.File]::WriteAllText((Join-Path $gameBin 'SingularityVR.ini'), 'after-ini')
& pwsh -NoProfile -File $guard -Action Rollback -SteamRoot $root -BackupDirectory $backup -ConfirmRollback | Out-Null
$pass = ((Get-Content (Join-Path $gameBin 'd3d9.dll') -Raw) -eq 'before-dll') -and ((Get-Content (Join-Path $gameBin 'SingularityVR.ini') -Raw) -eq 'before-ini') -and ((Get-Content (Join-Path $gameBin 'SingularityVfx.dll') -Raw) -eq 'before-vfx')
Remove-Item -LiteralPath $root -Recurse -Force
if (-not $pass) { throw 'Rollback content mismatch.' }
Write-Output 'PASS: synthetic backup/rollback restored all files and hashes.'
