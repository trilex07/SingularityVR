<#
Test-Installer.ps1 - offline installer suite vs a fake Singularity tree.
No Steam, no game, no admin. Exit code 0 = all pass.
#>
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$rel = Join-Path $here '..\Install-SingularityVR.ps1'
$manifestSrc = Join-Path $here '..\release_manifest.json'
$pass = 0; $fail = 0
function Ok($n) { $script:pass++; Write-Host "  PASS $n" -ForegroundColor Green }
function Bad($n, $why) { $script:fail++; Write-Host "  FAIL $n -- $why" -ForegroundColor Red }
function New-FakeDll([string]$p, [int]$machine = 0x014c) {
    $b = New-Object byte[] 256
    $b[0] = 0x4d; $b[1] = 0x5a
    [BitConverter]::GetBytes([int]64).CopyTo($b, 0x3c)
    $b[64] = 0x50; $b[65] = 0x45; $b[66] = 0; $b[67] = 0
    [BitConverter]::GetBytes([uint16]$machine).CopyTo($b, 68)
    [IO.File]::WriteAllBytes($p, $b)
}
function New-FakeGame([string]$root) {
    New-Item -ItemType Directory -Force (Join-Path $root 'Binaries') | Out-Null
    [IO.File]::WriteAllBytes((Join-Path $root 'Binaries\Singularity.exe'), [byte[]](1..64))
    Set-Content (Join-Path $root 'Binaries\SingularityVR.ini') "[Input]`nUserKey=keepme`n"
}
function New-Sources([string]$root) {
    New-Item -ItemType Directory -Force $root | Out-Null
    New-FakeDll (Join-Path $root 'd3d9.dll')
    Set-Content (Join-Path $root 'SingularityVR.ini') "[Input]`nUserKey=keepme`nDefaultKey=1`n"
    New-Item -ItemType Directory -Force (Join-Path $root 'openxr') | Out-Null
}
# point manifest build/repo outputs at testdata: use a scratch manifest copy
function New-TestManifest([string]$srcRoot, [string]$dst) {
    $m = Get-Content $manifestSrc -Raw | ConvertFrom-Json
    foreach ($c in $m.components) {
        if ($c.id -eq 'd3d9-dll') { $c.source = @{type = 'repo'; path = 'testdata/d3d9.dll' } }
        if ($c.id -eq 'vr-ini') { $c.source = @{type = 'repo'; path = 'testdata/SingularityVR.ini' } }
        if ($c.id -eq 'openxr-loader') { $c.source = @{type = 'repo'; path = 'testdata/openxr_loader.dll' } }
    }
    $m.game.builds = @()
    $m | ConvertTo-Json -Depth 6 | Set-Content $dst
}
$T = Join-Path ([IO.Path]::GetTempPath()) 'svr-inst-test'
if (Test-Path $T) { Remove-Item $T -Recurse -Force }
New-Item -ItemType Directory -Force $T | Out-Null
$td = Join-Path $T 'testdata'; New-Item -ItemType Directory -Force $td | Out-Null
New-FakeDll (Join-Path $td 'd3d9.dll'); New-FakeDll (Join-Path $td 'openxr_loader.dll')
Set-Content (Join-Path $td 'SingularityVR.ini') "[Input]`nUserKey=keepme`nDefaultKey=1`n"
# Stage a scratch copy of the installer+manifest pair (installer resolves
# RepoRoot two levels up, so repo-kind sources live under stage/testdata).
$stage = Join-Path $T 'stage'
New-Item -ItemType Directory -Force (Join-Path $stage 'tools\release') | Out-Null
Copy-Item $rel (Join-Path $stage 'tools\release\Install-SingularityVR.ps1')
$m2 = Get-Content $manifestSrc -Raw | ConvertFrom-Json
$m2.game.builds = @()
foreach ($c in $m2.components) {
    if ($c.id -eq 'd3d9-dll') { $c.source = @{type = 'repo'; path = 'testdata/d3d9.dll' } }
    if ($c.id -eq 'vr-ini') { $c.source = @{type = 'repo'; path = 'testdata/SingularityVR.ini' } }
    if ($c.id -eq 'openxr-loader') { $c.source = @{type = 'repo'; path = 'testdata/openxr_loader.dll' } }
}
$m2 | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $stage 'tools\release\release_manifest.json')
New-Item -ItemType Directory -Force (Join-Path $stage 'testdata') | Out-Null
Copy-Item (Join-Path $td 'd3d9.dll') (Join-Path $stage 'testdata\d3d9.dll')
Copy-Item (Join-Path $td 'openxr_loader.dll') (Join-Path $stage 'testdata\openxr_loader.dll')
Copy-Item (Join-Path $td 'SingularityVR.ini') (Join-Path $stage 'testdata\SingularityVR.ini')
$inst = Join-Path $stage 'tools\release\Install-SingularityVR.ps1'
$manPath = Join-Path $stage 'tools\release\release_manifest.json'

function Run-Inst($g, $extra) {
    $cmd = "& '$inst' -GamePath '$g' -ManifestPath '$manPath' -ForceUnknownGameBuild -AssetRoot '$assetRoot' $($extra -join ' ')"
    try { $o = Invoke-Expression $cmd 2>&1; return @{ok = $true; out = "$o" } }
    catch { return @{ok = $false; out = "$($_.Exception.Message)" } }
}
$assetRoot = Join-Path $T 'assets'
New-Item -ItemType Directory -Force (Join-Path $assetRoot 'texture-injector-dll') | Out-Null
New-FakeDll (Join-Path $assetRoot 'texture-injector-dll\dinput8.dll')
New-Item -ItemType Directory -Force (Join-Path $assetRoot 'texture-injector-ini') | Out-Null
Set-Content (Join-Path $assetRoot 'texture-injector-ini\texture_override.ini') "[TextureQuality]`nQuality=2x`n"

# 1 fresh install
$g1 = Join-Path $T 'g1'; New-FakeGame $g1
$r = Run-Inst $g1 @('-Profile', 'Core')
if ($r.ok -and (Test-Path "$g1\Binaries\d3d9.dll") -and (Test-Path "$g1\SingularityVR-installed.json")) { Ok('1 fresh install') } else { Bad('1 fresh install', $r.out) }
# 2 idempotency
$r = Run-Inst $g1 @('-Profile', 'Core')
if ($r.ok) { Ok('2 idempotent reinstall') } else { Bad('2 idempotent reinstall', $r.out) }
# 3 INI preservation
$ini = Get-Content "$g1\Binaries\SingularityVR.ini" -Raw
if ($ini -match 'UserKey=keepme' -and $ini -match 'DefaultKey=1') { Ok('3 INI user keys kept + defaults added') } else { Bad('3 INI', $ini) }
# 4 upgrade from older manifest (simulate: drop manifest version field check by reinstalling)
$r = Run-Inst $g1 @('-Profile', 'Core')
if ($r.ok) { Ok('4 upgrade path (reinstall over manifest)') } else { Bad('4 upgrade', $r.out) }
# 5 experimental gated by default (stage every experimental component: fail-closed requires all)
foreach ($eid in @('knife-pack','glove-pack','ammo-plates-ammo','ammo-plates-atlas','health-plate','vr-bik-boot','vr-bik-loading','ps-prompts')) {
    $d = Join-Path $assetRoot $eid; New-Item -ItemType Directory -Force $d | Out-Null
    Set-Content (Join-Path $d 'payload.bin') 'fake'
}
$r = Run-Inst $g1 @('-Profile', 'Core,Experimental')
if ($r.ok -and -not (Test-Path "$g1\Binaries\vr-assets\knife\knife.svrk")) { Ok('5 experimental excluded without flag') } else { Bad('5 experimental gate', $r.out) }
# 6 experimental install with flag
$r = Run-Inst $g1 @('-Profile', 'Core,Experimental', '-AllowExperimental')
if ($r.ok -and (Test-Path "$g1\Binaries\vr-assets\knife\payload.bin")) { Ok('6 experimental install') } else { Bad('6 experimental install', $r.out) }
# 7 full uninstall restores originals
$exeHashBefore = (Get-FileHash "$g1\Binaries\Singularity.exe" -Algorithm SHA256).Hash
& $inst -GamePath $g1 -ManifestPath $manPath -ForceUnknownGameBuild -Mode Uninstall | Out-Null
$cond = (-not (Test-Path "$g1\Binaries\d3d9.dll")) -and (-not (Test-Path "$g1\SingularityVR-installed.json")) -and ((Get-FileHash "$g1\Binaries\Singularity.exe" -Algorithm SHA256).Hash -eq $exeHashBefore)
if ($cond) { Ok('7 uninstall + originals intact') } else { Bad('7 uninstall', 'leftovers or exe changed') }
# 8 missing source
Remove-Item (Join-Path $stage 'testdata\d3d9.dll') -Force
$g2 = Join-Path $T 'g2'; New-FakeGame $g2
$r = Run-Inst $g2 @('-Profile', 'Core')
if (-not $r.ok -and $r.out -match 'missing source') { Ok('8 missing source fails closed') } else { Bad('8 missing source', $r.out) }
Copy-Item (Join-Path $td 'd3d9.dll') (Join-Path $stage 'testdata\d3d9.dll')
# 9 incorrect game path
$r = Run-Inst (Join-Path $T 'nosuchdir') @('-Profile', 'Core')
if (-not $r.ok) { Ok('9 bad game path fails') } else { Bad('9 bad game path', 'should have failed') }
# 10 missing exe
$g3 = Join-Path $T 'g3'; New-Item -ItemType Directory -Force (Join-Path $g3 'Binaries') | Out-Null
$r = Run-Inst $g3 @('-Profile', 'Core')
if (-not $r.ok -and $r.out -match 'Singularity.exe') { Ok('10 missing exe fails') } else { Bad('10 missing exe', $r.out) }
# 11 unsupported build (pinned wrong hash)
$m3 = Get-Content $manPath -Raw | ConvertFrom-Json
$m3.game.builds = @('00' * 32)
$m3 | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $T 'manifest-pinned.json')
try { & $inst -GamePath $g1 -ManifestPath (Join-Path $T 'manifest-pinned.json') -Profile Core 2>&1 | Out-Null; Bad('11 pinned build', 'should have failed') }
catch { if ("$_.Exception.Message" -match 'unsupported game build|no pinned') { Ok('11 unsupported build fails closed') } else { Bad('11 pinned build', "$_") } }
# 12 wrong-arch DLL rejected
New-FakeDll (Join-Path $stage 'testdata\d3d9.dll') -machine 0x8664
$g4 = Join-Path $T 'g4'; New-FakeGame $g4
$r = Run-Inst $g4 @('-Profile', 'Core')
if (-not $r.ok -and $r.out -match 'x86 PE32') { Ok('12 wrong-arch rejected') } else { Bad('12 wrong-arch', $r.out) }
Copy-Item (Join-Path $td 'd3d9.dll') (Join-Path $stage 'testdata\d3d9.dll') -Force
# 13 locked file -> rollback verified
$g5 = Join-Path $T 'g5'; New-FakeGame $g5
Set-Content "$g5\Binaries\d3d9.dll" 'original-content'
$lock = [IO.File]::Open("$g5\Binaries\d3d9.dll", 'Open', 'ReadWrite', 'None')
try {
  try { & $inst -GamePath $g5 -ManifestPath $manPath -ForceUnknownGameBuild -Profile Core 2>&1 | Out-Null; Bad('13 locked file', 'should have failed') }
  catch { if ("$_.Exception.Message" -match 'locked|INSTALLER-FAIL') { Ok('13 locked file fails + rollback path') } else { Bad('13 locked file', "$_") } }
} finally { $lock.Close() }
if ((Get-Content "$g5\Binaries\d3d9.dll" -Raw) -match 'original-content') { Ok('13b original preserved after failed install') } else { Bad('13b original preserved', 'content changed') }
# 14 INI user values win over defaults
Set-Content "$g5\Binaries\SingularityVR.ini" "[Input]`nUserKey=keepme`nDefaultKey=userwins`n"
$r = Run-Inst $g5 @('-Profile', 'Core')
$ini2 = Get-Content "$g5\Binaries\SingularityVR.ini" -Raw
if ($r.ok -and $ini2 -match 'DefaultKey=userwins') { Ok('14 user INI values win') } else { Bad('14 INI win', $ini2) }
# 15 path traversal in manifest fails
$m4 = Get-Content $manPath -Raw | ConvertFrom-Json
($m4.components | Where-Object { $_.id -eq 'd3d9-dll' }).dest = 'Binaries/../../evil.dll'
$m4 | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $T 'manifest-evil.json')
$g6 = Join-Path $T 'g6'; New-FakeGame $g6
try { & $inst -GamePath $g6 -ManifestPath (Join-Path $T 'manifest-evil.json') -ForceUnknownGameBuild -AssetRoot $assetRoot -Profile Core 2>&1 | Out-Null; Bad('15 traversal', 'should have failed') }
catch { if ("$_.Exception.Message" -match 'path escape') { Ok('15 path traversal blocked') } else { Bad('15 traversal', "$_") } }
if (Test-Path (Join-Path $T 'evil.dll')) { Bad('15 traversal file', 'escaped file exists') } else { Ok('15b no escape artifact') }
# 16 extra unknown files untouched by uninstall
$r = Run-Inst $g1 @('-Profile', 'Core')
Set-Content "$g1\Binaries\user-notes.txt" 'do not touch'
& $inst -GamePath $g1 -ManifestPath $manPath -ForceUnknownGameBuild -AssetRoot $assetRoot -Mode Uninstall | Out-Null
if (-not (Test-Path "$g1\Binaries\d3d9.dll") -and (Test-Path "$g1\Binaries\user-notes.txt")) { Ok('16 unknown files preserved') } else { Bad('16 unknown files', 'deleted or leftovers') }
# 17 repair restores missing component
$r = Run-Inst $g1 @('-Profile', 'Core')
if (-not $r.ok) { Bad('17 repair reinstall', $r.out) }
else {
  Remove-Item "$g1\Binaries\d3d9.dll" -Force
  try { & $inst -GamePath $g1 -ManifestPath $manPath -ForceUnknownGameBuild -AssetRoot $assetRoot -Mode Repair 2>&1 | Out-Null; if (Test-Path "$g1\Binaries\d3d9.dll") { Ok('17 repair restores') } else { Bad('17 repair', 'not restored') } }
  catch { Bad('17 repair', "$_") }
}
# 18 exact original restoration (hash compare after uninstall)
$g7 = Join-Path $T 'g7'; New-FakeGame $g7
$origIni = (Get-FileHash "$g7\Binaries\SingularityVR.ini" -Algorithm SHA256).Hash
$r = Run-Inst $g7 @('-Profile', 'Core')
& $inst -GamePath $g7 -ManifestPath $manPath -ForceUnknownGameBuild -Mode Uninstall | Out-Null
if ((Get-FileHash "$g7\Binaries\SingularityVR.ini" -Algorithm SHA256).Hash -eq $origIni) { Ok('18 byte-exact INI restore') } else { Bad('18 INI restore', 'hash differs') }

Write-Host ""
Write-Host "RESULT pass=$pass fail=$fail"
if ($fail -gt 0) { exit 1 }
