# Build libbink binkenc (BIKi encoder) offline. Does not touch Steam or game installs.
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$Zip = Join-Path (Split-Path -Parent $Root) 'third_party/libbink-wayback.zip'
$Src = Join-Path (Split-Path -Parent $Root) 'third_party/libbink-src/libbink-main'
if (-not (Test-Path -LiteralPath $Zip)) {
    throw "Missing $Zip — fetch Wayback snapshot of jmarshall23/libbink (GitHub HTTPS clone returns 404 on this host)."
}
if (-not (Test-Path -LiteralPath $Src)) {
    Expand-Archive -Force $Zip (Join-Path (Split-Path -Parent $Root) 'third_party/libbink-src')
}
dotnet build (Join-Path $Src 'encoder/LibBink.Encoder.sln') -c Release
$binkenc = Join-Path $Src 'encoder/src/LibBink.Encoder/bin/Release/net8.0/binkenc.dll'
if (-not (Test-Path -LiteralPath $binkenc)) { throw "Build failed: $binkenc" }
Write-Output $binkenc
