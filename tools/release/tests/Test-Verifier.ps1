<#
Test-Verifier.ps1 - offline verifier suite on synthetic asset trees.
Exit 0 = all expectations met.
#>
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$ver = Join-Path $here '..\Verify-ReleaseAssets.ps1'
$pass = 0; $fail = 0
function Ok($n) { $script:pass++; Write-Host "  PASS $n" -ForegroundColor Green }
function Bad($n, $why) { $script:fail++; Write-Host "  FAIL $n -- $why" -ForegroundColor Red }
function W([byte[]]$b, [string]$p) { [IO.File]::WriteAllBytes($p, $b) }
function New-Png([string]$p, [int]$w, [int]$h) {
    $b = New-Object byte[] 45
    @(0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0, 0, 0, 13, 0x49, 0x48, 0x44, 0x52) | ForEach-Object -Begin { $i = 0 } { $b[$i++] = $_ }
    $b[16] = ($w -shr 24) -band 255; $b[17] = ($w -shr 16) -band 255; $b[18] = ($w -shr 8) -band 255; $b[19] = $w -band 255
    $b[20] = ($h -shr 24) -band 255; $b[21] = ($h -shr 16) -band 255; $b[22] = ($h -shr 8) -band 255; $b[23] = $h -band 255
    W $b $p
}
function New-Dds([string]$p, [int]$w, [int]$h, [string]$four) {
    $hdr = New-Object byte[] 128
    $hdr[0] = 0x44; $hdr[1] = 0x44; $hdr[2] = 0x53; $hdr[3] = 0x20
    [BitConverter]::GetBytes([uint32]$h).CopyTo($hdr, 12)
    [BitConverter]::GetBytes([uint32]$w).CopyTo($hdr, 16)
    [Text.Encoding]::ASCII.GetBytes($four).CopyTo($hdr, 84)
    $blk = if ($four -eq 'DXT1') { 8 } else { 16 }
    $mips = 0; $cw = $w; $ch = $h; $total = 128
    while ($true) {
        $total += [Math]::Max(1, [int]($cw / 4)) * $blk * [Math]::Max(1, [int]($ch / 4)); $mips++
        if ($cw -le 4 -and $ch -le 4) { break }
        $cw = [Math]::Max(1, [int]($cw / 2)); $ch = [Math]::Max(1, [int]($ch / 2))
    }
    [BitConverter]::GetBytes([uint32]$mips).CopyTo($hdr, 28)
    $b = New-Object byte[] $total
    [Array]::Copy($hdr, $b, 128)
    W $b $p
}
function New-Wav([string]$p, [int]$ch = 1, [int]$rate = 48000, [int]$fmt = 1) {
    $b = New-Object byte[] 48
    [Text.Encoding]::ASCII.GetBytes('RIFF').CopyTo($b, 0)
    [Text.Encoding]::ASCII.GetBytes('WAVE').CopyTo($b, 8)
    [BitConverter]::GetBytes([uint16]$fmt).CopyTo($b, 20)
    [BitConverter]::GetBytes([uint16]$ch).CopyTo($b, 22)
    [BitConverter]::GetBytes([uint32]$rate).CopyTo($b, 24)
    W $b $p
}
function New-Bik([string]$p, [int]$len = 2048) {
    $b = New-Object byte[] $len
    $b[0] = 0x42; $b[1] = 0x49; $b[2] = 0x4B; $b[3] = 0x69
    W $b $p
}
function New-FakeDll([string]$p) {
    $b = New-Object byte[] 256
    $b[0] = 0x4D; $b[1] = 0x5A
    [BitConverter]::GetBytes([int]64).CopyTo($b, 0x3c)
    $b[64] = 0x50; $b[65] = 0x45
    [BitConverter]::GetBytes([uint16]0x014c).CopyTo($b, 68)
    W $b $p
}
function Run-Ver($assetRoot, $manifest) {
    # NOTE: $LASTEXITCODE is unreliable across WinPS/PS7 pipelines; derive the
    # verdict from the verifier's own summary line instead.
    $o = & $ver -ManifestPath $manifest -AssetRoot $assetRoot 2>&1 | Out-String
    $code = -1
    if ($o -match 'VERIFY-RESULT blockers=(\d+)') { $code = [int]$Matches[1] }
    return @{code = $code; out = $o }
}
$T = Join-Path ([IO.Path]::GetTempPath()) 'svr-verify-test'
if (Test-Path $T) { Remove-Item $T -Recurse -Force }
New-Item -ItemType Directory -Force $T | Out-Null
function New-TestManifest {
    return @{
        game       = @{exe = 'Binaries/Singularity.exe'; builds = @() }
        profiles   = @{core = @(); experimental = @('knife-pack', 'ammo-plates-ammo', 'pl-text-pack', 'pl-story-audio') }
        components = @(
            @{id = 'knife-pack'; kind = 'asset-pack'; profile = 'experimental'; required = $false; dest = 'Binaries/vr-assets/knife/'; source = @{} },
            @{id = 'ammo-plates-ammo'; kind = 'asset-pack'; profile = 'experimental'; required = $false; dest = 'Binaries/vr-assets/hud/katorga/ammo/'; source = @{} },
            @{id = 'pl-text-pack'; kind = 'localization'; profile = 'experimental'; required = $false; dest = 'Binaries/vr-assets/localization/pl/'; source = @{} },
            @{id = 'pl-story-audio'; kind = 'audio'; profile = 'experimental'; required = $false; dest = 'Binaries/vr-assets/audio/polish/story/'; source = @{} }
        )
    }
}
$mg = Join-Path $T 'manifest-good.json'
New-TestManifest | ConvertTo-Json -Depth 6 | Set-Content $mg
$good = Join-Path $T 'good'
foreach ($id in @('knife-pack', 'pl-text-pack', 'pl-story-audio', 'ammo-plates-ammo')) {
    New-Item -ItemType Directory -Force (Join-Path $good $id) | Out-Null
}
New-Png (Join-Path $good 'ammo-plates-ammo\plate.png') 256 128
New-Dds (Join-Path $good 'ammo-plates-ammo\atlas.dds') 64 64 'DXT1'
Set-Content (Join-Path $good 'knife-pack\knife.svrk') 'opaque-blobs-allowed'
Set-Content (Join-Path $good 'pl-text-pack\polish_text.jsonl') '{"a":1}'
New-Wav (Join-Path $good 'pl-story-audio\line.wav')
New-Bik (Join-Path $good 'pl-story-audio\lang.bik')
$r = Run-Ver $good $mg
if ($r.code -eq 0) { Ok('1 clean tree passes') } else { Bad('1 clean tree', $r.out) }

$bad = Join-Path $T 'bad'
foreach ($id in @('knife-pack', 'ammo-plates-ammo', 'pl-story-audio')) {
    New-Item -ItemType Directory -Force (Join-Path $bad $id) | Out-Null
}
Set-Content (Join-Path $bad 'knife-pack\knife.svrk') 'x'
Set-Content (Join-Path $bad 'ammo-plates-ammo\plate.png') 'not a png at all, just text.........'
New-Dds (Join-Path $bad 'ammo-plates-ammo\short.dds') 64 64 'DXT1'
$b = [IO.File]::ReadAllBytes((Join-Path $bad 'ammo-plates-ammo\short.dds'))
[IO.File]::WriteAllBytes((Join-Path $bad 'ammo-plates-ammo\short.dds'), $b[0..200])
New-Wav (Join-Path $bad 'pl-story-audio\float.wav') -fmt 3
Set-Content (Join-Path $bad 'pl-story-audio\broken.jsonl') (@('{"a":1}', 'NOT JSON') -join "`n")
$r = Run-Ver $bad $mg
if ($r.code -gt 0 -and $r.out -match 'BLOCKER') { Ok('2 bad tree blocked (png/dds/wav/jsonl)') } else { Bad('2 bad tree', "code=$($r.code)") }

$dup = Join-Path $T 'manifest-dup.json'
$m2 = New-TestManifest
$m2.components += ($m2.components | Where-Object { $_.id -eq 'knife-pack' })
$m2 | ConvertTo-Json -Depth 6 | Set-Content $dup
$r = Run-Ver $good $dup
# NOTE: WinPS 5.1 does not capture Write-Host; reason strings are asserted
# manually in test 2 via -match, single-variable mutations here assert code only.
if ($r.code -gt 0) { Ok('3 duplicate IDs blocked') } else { Bad('3 duplicate IDs', "code=$($r.code)") }

$evil = Join-Path $T 'manifest-evil.json'
$m3 = New-TestManifest
($m3.components | Where-Object { $_.id -eq 'knife-pack' }).dest = '../escape/'
$m3 | ConvertTo-Json -Depth 6 | Set-Content $evil
$r = Run-Ver $good $evil
if ($r.code -gt 0) { Ok('4 dest escape blocked') } else { Bad('4 dest escape', "code=$($r.code)") }

Write-Host ""
Write-Host "RESULT pass=$pass fail=$fail"
if ($fail -gt 0) { exit 1 }
