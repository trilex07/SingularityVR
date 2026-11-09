# Read-only game inspection. No game launch, install, registry or save changes.
param(
    [Parameter(Mandatory=$true)][string]$GameRoot,
    [string]$OutputDirectory = (Join-Path $env:TEMP ('SingularityVR-diagnostics-' + (Get-Date -Format 'yyyyMMdd-HHmmss')))
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path -LiteralPath $GameRoot).Path
if (!(Test-Path -LiteralPath (Join-Path $root 'Binaries\Singularity.exe'))) { throw 'Expected game root containing Binaries\Singularity.exe' }
if ((Test-Path -LiteralPath $OutputDirectory) -or (Test-Path -LiteralPath ($OutputDirectory + '.zip'))) { throw 'Choose a fresh output directory' }
$out = New-Item -ItemType Directory -Path $OutputDirectory
$receipt = [ordered]@{ collectedAt=(Get-Date).ToString('o'); gameRoot=$root; files=@(); logs=@(); windowsEvents=''; processes=@() }
foreach ($relative in @('Binaries\Singularity.exe','Binaries\d3d9.dll','Binaries\dinput8.dll',
    'Binaries\dinput8_orig.dll','Binaries\binkw32.dll','Binaries\ogg.dll','Binaries\vorbis.dll',
    'Binaries\vorbisfile.dll','Binaries\openxr_loader.dll','Binaries\SingularityVR.ini',
    'RvGame\CookedPC\wp_SP_ARShotgun_SF.xxx')) {
    $path = Join-Path $root $relative
    if (Test-Path -LiteralPath $path -PathType Leaf) {
        $f = Get-Item -LiteralPath $path
        $receipt.files += @{ path=$relative; bytes=$f.Length; sha256=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash }
    } else { $receipt.files += @{ path=$relative; missing=$true } }
}
$sources = @(
    @{name='mod'; path=(Join-Path $env:LOCALAPPDATA 'SingularityVR'); filter='view_matrix*.log'},
    @{name='game'; path=(Join-Path $root 'RvGame\Logs'); filter='*.log'},
    @{name='texture'; path=(Join-Path $root 'Binaries'); filter='texture_override.log'}
)
foreach ($source in $sources) {
    $logs = @(Get-ChildItem -LiteralPath $source.path -Filter $source.filter -File -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -First 10)
    if (!$logs.Count) { $receipt.logs += @{ source=$source.name; missing=$true }; continue }
    $dest = New-Item -ItemType Directory -Path (Join-Path $out.FullName $source.name)
    foreach ($f in $logs) {
        try {
            Copy-Item -LiteralPath $f.FullName -Destination $dest.FullName
            $receipt.logs += @{ source=$source.name; name=$f.Name; bytes=$f.Length; modified=$f.LastWriteTime.ToString('o') }
        } catch { $receipt.logs += @{ source=$source.name; name=$f.Name; error=$_.Exception.Message } }
    }
}
try {
    $events = @(Get-WinEvent -FilterHashtable @{LogName='Application'; Id=1000,1001; StartTime=(Get-Date).AddDays(-7)} -ErrorAction Stop |
        Where-Object { $_.Message -match '(?i)\bsingularity\.exe\b' } | Select-Object -First 20)
    $events | ForEach-Object { $_.ToXml() } | Set-Content -LiteralPath (Join-Path $out.FullName 'windows-events.xml') -Encoding UTF8
    $receipt.windowsEvents = 'matching events: ' + $events.Count
} catch { $receipt.windowsEvents = $_.Exception.Message }
$receipt.processes = @(Get-Process -Name Singularity -ErrorAction SilentlyContinue | Select-Object Id,ProcessName)
$receipt | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $out.FullName 'manifest.json') -Encoding UTF8
Compress-Archive -Path (Join-Path $out.FullName '*') -DestinationPath ($out.FullName + '.zip')
Write-Output ('Diagnostics: ' + $out.FullName + '.zip')
