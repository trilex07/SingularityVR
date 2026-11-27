# Run with Singularity already open at the wreck. An existing log can also be
# collected after exit, provided its matching trace and bitmap are beside it.
[CmdletBinding()]
param(
    [string]$ExistingLog,
    [string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

function Read-SharedLog([string]$Path, [long]$Offset = 0) {
    $stream = [IO.File]::Open($Path, 'Open', 'Read', 'ReadWrite')
    try {
        if ($stream.Length -lt $Offset) { throw 'The game log restarted during the capture.' }
        $stream.Position = $Offset
        $reader = [IO.StreamReader]::new($stream)
        try { return $reader.ReadToEnd() } finally { $reader.Dispose() }
    } finally { $stream.Dispose() }
}

function Find-FramePair([string]$Log, [string]$ProcessPattern) {
    $armed = $Log.LastIndexOf("shadow-pass trace armed:", [StringComparison]::Ordinal)
    if ($armed -lt 0) { return }
    $requested = $Log.Substring($armed)
    $finished = [regex]::Match($requested, "(?m)^shadow-pass trace finished [^\r\n]* file=[^\r\n]*[\\/](?<trace>shadow-pass-${ProcessPattern}-\d+\.txt)\r?$")
    if (!$finished.Success) { return }
    $image = [regex]::Match($requested.Substring($finished.Index + $finished.Length), '^\r?\ncapture: saved [^\r\n]*[\\/](?<bitmap>capture_[\d_-]+\.bmp) \(\d+x\d+\) source=XR-backbuffer\r?(?:\n|$)')
    if ($image.Success) {
        return [pscustomobject]@{ Trace = $finished.Groups['trace'].Value; Bitmap = $image.Groups['bitmap'].Value }
    }
}

if ($ExistingLog) {
    $log = (Get-Item -LiteralPath $ExistingLog).FullName
    $processPattern = '\d+'
    $offset = 0
} else {
    $games = @(Get-Process -Name Singularity -ErrorAction SilentlyContinue)
    if ($games.Count -ne 1) { throw 'Open Singularity VR and load the campaign at the wreck first.' }
    $game = $games[0]
    $log = Join-Path $env:LOCALAPPDATA 'SingularityVR\view_matrix.log'
    $processPattern = [string]$game.Id
    Write-Host 'Return to the headset and hold both hands forward where the extra helicopter is visible. Capture starts in 15 seconds.'
    Start-Sleep -Seconds 15
    if ($game.HasExited) { throw 'The game closed before the capture.' }
    $offset = (Get-Item -LiteralPath $log).Length
    $request = Join-Path (Split-Path $log) 'shadow-pass.request'
    if ([IO.File]::Exists($request)) { throw 'A capture is already pending. Return to the wreck and collect the resulting log using -ExistingLog.' }
    $marker = [IO.File]::Open($request, 'CreateNew', 'Write', 'None')
    $marker.Dispose()
}
$dir = Split-Path $log
$deadline = [DateTime]::UtcNow.AddSeconds(75)
do {
    if (!$ExistingLog -and $game.HasExited) { throw 'The game closed. Run with -ExistingLog and the log from this session.' }
    $pair = Find-FramePair (Read-SharedLog $log $offset) $processPattern
    if ($pair) { break }
    if ($ExistingLog) { throw 'The most recent manual request has no completed trace with an adjacent XR-backbuffer capture in this log.' }
    Start-Sleep -Milliseconds 400
} while ([DateTime]::UtcNow -lt $deadline)
if (!$pair) { throw 'No paired frame arrived. Stay in focused VR gameplay with both controllers tracked. A pending request may still complete.' }

if (!$OutputDirectory) { $OutputDirectory = $dir }
$out = (Get-Item -LiteralPath $OutputDirectory).FullName
$stem = 'SingularityVR-' + [IO.Path]::GetFileNameWithoutExtension($pair.Trace)
$final = Join-Path $out ($stem + '.zip')
if ([IO.File]::Exists($final)) { $final = Join-Path $out ($stem + '-' + [guid]::NewGuid().ToString('N') + '.zip') }
$temp = Join-Path $out ([guid]::NewGuid().ToString('N') + '.tmp')
$zip = [IO.Compression.ZipFile]::Open($temp, 'Create')
try {
    try {
        foreach ($name in @($pair.Trace, $pair.Bitmap)) {
            $path = Join-Path $dir $name
            if ((Get-Item -LiteralPath $path).Length -le 0) { throw "Empty capture file: $name" }
            [void][IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, $path, $name)
        }
        $snapshot = Read-SharedLog $log
        $bitmapName = [regex]::Escape($pair.Bitmap)
        if ([regex]::Matches($snapshot, "(?m)^capture: saved [^\r\n]*[\\/]${bitmapName} ").Count -ne 1) {
            throw 'The bitmap name is missing or was reused. Capture another frame.'
        }
        $entry = $zip.CreateEntry('view_matrix.log')
        $writer = [IO.StreamWriter]::new($entry.Open())
        try {
            $writer.Write([regex]::Replace($snapshot, [regex]::Escape($env:USERPROFILE), '%USERPROFILE%', 'IgnoreCase'))
        } finally { $writer.Dispose() }
    } finally { $zip.Dispose() }
    [IO.File]::Move($temp, $final)
} finally {
    if ([IO.File]::Exists($temp)) { [IO.File]::Delete($temp) }
}
Write-Output $final
