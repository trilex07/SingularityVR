param(
    [string]$RepoRoot = (Split-Path -Parent $PSScriptRoot),
    [string]$GameMovies = 'C:\Program Files (x86)\Steam\steamapps\common\Singularity\RvGame\Movies',
    [string]$OutRoot = (Join-Path (Split-Path -Parent $RepoRoot) 'deployment/hands-dubbing/boot-logo-v1'),
    [string]$FfmpegBin = "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\Gyan.FFmpeg_Microsoft.Winget.Source_8wekyb3d8bbwe\ffmpeg-9.0.2-full_build\bin"
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (Test-Path -LiteralPath $OutRoot) { throw "Refuse existing output root: $OutRoot" }
$binkenc = & (Join-Path $RepoRoot 'tools/build-binkenc.ps1')
$ff = Join-Path $FfmpegBin 'ffmpeg.exe'
$fp = Join-Path $FfmpegBin 'ffprobe.exe'
New-Item -ItemType Directory -Path $OutRoot | Out-Null
$candidate = Join-Path $OutRoot 'candidate'
python (Join-Path $RepoRoot 'tools/title/prepare_boot_logo_bink.py') `
    --source (Join-Path $GameMovies 'SNG_logo.bik') `
    --out $candidate --ffmpeg $ff --ffprobe $fp --binkenc $binkenc
$vrBik = Join-Path $candidate 'SNG_logo_vr.bik'
python (Join-Path $RepoRoot 'tools/title/remux_bink_stock_audio.py') `
    --stock (Join-Path $GameMovies 'SNG_logo.bik') `
    --candidate-video $vrBik `
    --output (Join-Path $candidate 'SNG_logo_vr.remux.bik')
Move-Item -Force (Join-Path $candidate 'SNG_logo_vr.remux.bik') $vrBik
$binkMedia = Join-Path (Split-Path -Parent $RepoRoot) 'SingularityVR-bink-dubbing/tools/bink_media.py'
python $binkMedia validate-video `
    (Join-Path $GameMovies 'SNG_logo.bik') $vrBik --ffmpeg $ff --ffprobe $fp
Write-Output "Boot logo bundle (BIKi): $OutRoot"
