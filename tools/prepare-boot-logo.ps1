param(
    [string]$RepoRoot = (Split-Path -Parent $PSScriptRoot),
    [string]$GameMovies = 'C:\Program Files (x86)\Steam\steamapps\common\Singularity\RvGame\Movies',
    [string]$OutRoot = (Join-Path (Split-Path -Parent $PSScriptRoot) 'deployment/hands-dubbing/boot-logo-v1'),
    [string]$FfmpegBin = "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\Gyan.FFmpeg_Microsoft.Winget.Source_8wekyb3d8bbwe\ffmpeg-9.0.2-full_build\bin"
)
# Offline boot-logo SIDE pipeline. Does not install into Steam or touch DLL/INI/UI packages.
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (Test-Path -LiteralPath $OutRoot) { throw "Refuse existing output root: $OutRoot" }
$ui = Join-Path $RepoRoot 'deployment/hands-dubbing/post-opening/ui-polish-v1/control.u'
$logos = Join-Path $RepoRoot 'deployment/hands-dubbing/ui-source-re/stock/ui_logos_SF.u'
foreach ($p in @($ui, $logos, (Join-Path $GameMovies 'SNG_logo.bik'))) {
    if (-not (Test-Path -LiteralPath $p)) { throw "Missing prerequisite: $p" }
}
$ff = Join-Path $FfmpegBin 'ffmpeg.exe'
$fp = Join-Path $FfmpegBin 'ffprobe.exe'
if (-not (Test-Path -LiteralPath $ff)) { throw "ffmpeg not found at $FfmpegBin" }
New-Item -ItemType Directory -Path $OutRoot | Out-Null
python (Join-Path $RepoRoot 'tools/title/prove_boot_logo_producer.py') `
    --game-movies $GameMovies --ui-logical $ui --ui-logos-logical $logos `
    --ffmpeg $ff --ffprobe $fp --out (Join-Path $OutRoot 'producer-evidence.json')
python (Join-Path $RepoRoot 'tools/title/prepare_boot_logo_bink.py') `
    --source (Join-Path $GameMovies 'SNG_logo.bik') `
    --out (Join-Path $OutRoot 'candidate') --ffmpeg $ff --ffprobe $fp
Write-Output "Boot logo SIDE bundle: $OutRoot"
