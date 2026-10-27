# Known-good launch combo (no VC++ Runtime Error on Steam start).
# d3d9 = manual-20261009-context; dinput8 = texture-injector 54784 (939E…), NOT the 37376 (B810…) build.
[CmdletBinding()]
param(
    [string]$SteamRoot = 'C:\Program Files (x86)\Steam',
    [switch]$DisableTextureInjector
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$bin = Join-Path $SteamRoot 'steamapps\common\Singularity\Binaries'
$d3dSrc = Join-Path $repo 'deployment\install-backups\manual-20261009-context\d3d9.dll'
$dinputSrc = Join-Path $repo 'deployment\install-backups\20261009-114517\dinput8.dll'
$dinputAssets = Join-Path (Split-Path $repo -Parent) 'SingularityVR-assets\asset-tools\texture-injector\dinput8.dll'

if (-not (Test-Path -LiteralPath $d3dSrc)) { throw "Missing: $d3dSrc" }

Get-Process -Name Singularity -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 2

Copy-Item -LiteralPath $d3dSrc -Destination (Join-Path $bin 'd3d9.dll') -Force

if ($DisableTextureInjector) {
    $d = Join-Path $bin 'dinput8.dll'
    if (Test-Path -LiteralPath $d) {
        Rename-Item -LiteralPath $d -NewName 'dinput8.dll.disabled' -Force
    }
    Write-Host 'Texture injector OFF (dinput8.dll.disabled). VR mod still via d3d9.dll.'
} else {
    if (Test-Path -LiteralPath $dinputSrc) {
        Copy-Item -LiteralPath $dinputSrc -Destination (Join-Path $bin 'dinput8.dll') -Force
    } elseif (Test-Path -LiteralPath $dinputAssets) {
        $h = (Get-FileHash -LiteralPath $dinputAssets).Hash.ToUpperInvariant()
        if ($h -eq 'B810FA5256287A293382FF30DA9D0CC906D317F3570BCC7F3CA920B058AC9FDB') {
            Write-Warning 'asset-tools dinput8 is the 37376-byte build; it has crashed with newer d3d9. Use -DisableTextureInjector or restore from backup 20261009-114517.'
        }
        Copy-Item -LiteralPath $dinputAssets -Destination (Join-Path $bin 'dinput8.dll') -Force
    }
    if (-not (Test-Path -LiteralPath (Join-Path $bin 'dinput8_orig.dll'))) {
        Copy-Item -LiteralPath (Join-Path $env:windir 'SysWOW64\dinput8.dll') -Destination (Join-Path $bin 'dinput8_orig.dll') -Force
    }
}

$d3 = (Get-FileHash (Join-Path $bin 'd3d9.dll') -Algorithm SHA256).Hash
Write-Host "d3d9.dll  SHA-256: $d3"
Write-Host 'Expected stable d3d9: 0265224C623262C629CC2E52EF247EBDF7922F60E396B52500C60654DC24C0BD'
if (Test-Path -LiteralPath (Join-Path $bin 'dinput8.dll')) {
    $di = (Get-FileHash (Join-Path $bin 'dinput8.dll') -Algorithm SHA256).Hash
    Write-Host "dinput8.dll SHA-256: $di"
    Write-Host 'Expected stable dinput8: 939E46948B60CC64A029E17480364A65C09E8FAEAA185DE6A33D15CF45EF21F8 (54784 bytes)'
}
Write-Host 'Launch: tools\Start-SingularityVR-Headset.ps1'
