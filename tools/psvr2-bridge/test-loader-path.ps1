param([Parameter(Mandatory=$true)][string]$Bridge)
$ErrorActionPreference='Stop'
$Bridge=(Resolve-Path -LiteralPath $Bridge).Path
$fixture=Join-Path ([IO.Path]::GetTempPath()) ('psvr2-loader-Żółć-'+[Guid]::NewGuid().ToString('N'))
$prior=$env:LOCALAPPDATA
try {
    New-Item -ItemType Directory -Path $fixture | Out-Null
    $env:LOCALAPPDATA=$fixture
    $missing=Join-Path $fixture 'nonexistent-steam/loader.dll'
    if ((& $Bridge --resolve-loader $missing) -ne $missing) { throw 'Missing path was not preserved for diagnostics' }
    $installed=Join-Path $fixture 'Trilex/PSVR2Toolkit/capi/psvr2_toolkit_capi_loader.dll'
    New-Item -ItemType Directory -Path (Split-Path $installed) -Force | Out-Null
    [IO.File]::WriteAllText($installed,'path fixture only, never loaded')
    if ((& $Bridge --resolve-loader $missing) -ne $installed) { throw 'Per-user fallback failed' }
    $explicit=Join-Path $fixture 'custom-loader.dll'
    [IO.File]::WriteAllText($explicit,'explicit fixture only, never loaded')
    if ((& $Bridge --resolve-loader $explicit) -ne $explicit) { throw 'Explicit loader was overridden' }
    $env:LOCALAPPDATA=''
    if ((& $Bridge --resolve-loader $missing) -ne $missing) { throw 'Missing environment fallback failed' }
    Write-Output 'PASS: explicit loader, missing file, Unicode AppData fallback, absent environment; no CAPI or game initialized'
} finally {
    $env:LOCALAPPDATA=$prior
    # Keep the tiny fixture as a test receipt; never remove user directories.
}
