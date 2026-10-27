param([string]$OutputDirectory = '')

$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Resolve-Path (Join-Path $here '..\..')
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $root 'deployment\psvr2-bridge-x64' }
$OutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) { throw 'Visual Studio x64 C++ tools not found.' }
$vcvars = Join-Path $vsPath 'VC\Auxiliary\Build\vcvarsall.bat'
$source = Join-Path $here 'SingularityPsvr2Bridge.cpp'
$exe = Join-Path $OutputDirectory 'SingularityPsvr2Bridge.exe'
$cmd = '"{0}" x64 >nul && cl /nologo /O2 /W4 /EHsc /MD /std:c++17 /DUNICODE /D_UNICODE /I"{1}" /Fe:"{2}" "{3}"' -f $vcvars,$root,$exe,$source
$output = cmd /c $cmd
$output | Write-Host
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $exe -PathType Leaf)) { throw 'x64 bridge build failed.' }

$bytes = [System.IO.File]::ReadAllBytes($exe)
$pe = [BitConverter]::ToInt32($bytes,0x3c)
$machine = [BitConverter]::ToUInt16($bytes,$pe+4)
$magic = [BitConverter]::ToUInt16($bytes,$pe+24)
if ($machine -ne 0x8664 -or $magic -ne 0x20b) { throw 'Built bridge is not x64 PE32+.' }
Get-FileHash -LiteralPath $exe -Algorithm SHA256 | Format-List
Write-Host "PASS: x64 PE32+ bridge -> $exe"
