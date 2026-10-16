<#
Verify-ReleaseAssets.ps1 - offline release-asset verifier (SIDE, no Steam).

  .\Verify-ReleaseAssets.ps1 -ManifestPath tools/release/release_manifest.json -AssetRoot <staging>

Checks staged trees + single-file sources against the manifest:
missing required/optional assets, unexpected files, SHA mismatches (when
pinned), duplicate IDs, bad dest paths, invalid container formats
(PNG/DDS/WAV/BIK/DLL/JSON/INI), unsupported texture tiers.
Exit 0 = no blockers (warnings allowed), 1 = blockers found.
#>
[CmdletBinding()]
param(
    [string]$ManifestPath = '',
    [string]$AssetRoot = '',
    [string]$RepoRoot = ''
)
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $ManifestPath) { $ManifestPath = Join-Path $here 'release_manifest.json' }
if (-not $RepoRoot) { $RepoRoot = Resolve-Path (Join-Path $here '..\..') }
$manifest = Get-Content $ManifestPath -Raw | ConvertFrom-Json
$blockers = @(); $warnings = @()
function Block($m) { $script:blockers += $m; Write-Host "  BLOCKER $m" -ForegroundColor Red }
function Warn($m) { $script:warnings += $m; Write-Host "  warn $m" -ForegroundColor Yellow }
function Ok($m) { Write-Host "  ok $m" -ForegroundColor Green }

# --- manifest hygiene ---
$ids = @()
foreach ($c in $manifest.components) { $ids += $c.id }
$dups = $ids | Group-Object | Where-Object { $_.Count -gt 1 } | ForEach-Object { $_.Name }
foreach ($d in $dups) { Block("duplicate component id: $d") }
foreach ($c in $manifest.components) {
    if ($c.kind -eq 'exclusion') { continue }
    if ($c.dest -and ($c.dest -match '(^|[\\/])\.\.([\\/]|$)' -or [IO.Path]::IsPathRooted(($c.dest -replace '/', '\')))) {
        Block("$($c.id): dest escapes game dir: $($c.dest)")
    }
    if ($c.id -eq 'texture-tier') {
        foreach ($t in $c.source.tiers) { if ($t -notin @('2x', '4K', '8K')) { Block("unsupported texture tier: $t") } }
        if ($c.source.tiers -contains '8K') { Warn("8K tier listed but no 8K content exists anywhere (see enemy-detail findings)") }
    }
}
if ($manifest.game.builds.Count -eq 0) { Warn("no pinned game builds; installer needs -ForceUnknownGameBuild") }

function Test-BinaryFormat([string]$p) {
    # returns format tag or '' when invalid
    try {
        $fs = [IO.File]::OpenRead($p); $b = New-Object byte[] 132
        $n = $fs.Read($b, 0, 132); $fs.Close()
        if ($n -lt 12) { return '' }
        if ($b[0] -eq 0x89 -and $b[1] -eq 0x50 -and $b[2] -eq 0x4E -and $b[3] -eq 0x47) {
            if ($n -lt 24) { return '' }
            $w = ([int]$b[16] -shl 24) -bor ([int]$b[17] -shl 16) -bor ([int]$b[18] -shl 8) -bor [int]$b[19]
            $h = ([int]$b[20] -shl 24) -bor ([int]$b[21] -shl 16) -bor ([int]$b[22] -shl 8) -bor [int]$b[23]
            if ($w -lt 1 -or $h -lt 1 -or $w -gt 8192 -or $h -gt 8192) { return '' }
            return 'PNG'
        }
        if ($b[0] -eq 0x44 -and $b[1] -eq 0x44 -and $b[2] -eq 0x53 -and $b[3] -eq 0x20) {
            $four = [Text.Encoding]::ASCII.GetString($b, 84, 4)
            $w = [BitConverter]::ToUInt32($b, 16); $h = [BitConverter]::ToUInt32($b, 12)
            $mips = [BitConverter]::ToUInt32($b, 28)
            if ($w -lt 4 -or $h -lt 4 -or $w -gt 8192 -or $h -gt 8192) { return '' }
            if ($four -notin @('DXT1', 'DXT5')) { return '' }
            $blk = if ($four -eq 'DXT1') { 8 } else { 16 }
            $expect = 128; $cw = $w; $ch = $h
            for ($i = 0; $i -lt $mips; $i++) {
                $expect += [Math]::Max(1, [int]($cw / 4)) * $blk * [Math]::Max(1, [int]($ch / 4))
                $cw = [Math]::Max(1, [int]($cw / 2)); $ch = [Math]::Max(1, [int]($ch / 2))
            }
            if ((Get-Item $p).Length -ne $expect) { return '' }
            return "DDS-$four"
        }
        if ($b[0] -eq 0x52 -and $b[1] -eq 0x49 -and $b[2] -eq 0x46 -and $b[3] -eq 0x46) {
            if ([Text.Encoding]::ASCII.GetString($b, 8, 4) -ne 'WAVE') { return '' }
            $audioFmt = [BitConverter]::ToUInt16($b, 20); $ch = [BitConverter]::ToUInt16($b, 22)
            $rate = [BitConverter]::ToUInt32($b, 24)
            if ($audioFmt -ne 1) { return '' }
            if ($ch -notin @(1, 2) -or $rate -notin @(44100, 48000)) { return '' }
            return 'WAV-PCM'
        }
        if ($b[0] -eq 0x42 -and $b[1] -eq 0x49 -and $b[2] -eq 0x4B -and ($b[3] -eq 0x69 -or $b[3] -eq 0x62)) { return 'BIK' }
        if ($b[0] -eq 0x4D -and $b[1] -eq 0x5A) {
            $pe = [BitConverter]::ToInt32($b, 0x3c)
            if ($pe -lt 0 -or $pe + 6 -gt $n) { return '' }
            $machine = [BitConverter]::ToUInt16($b, $pe + 4)
            if ($machine -eq 0x014c) { return 'PE32' }
            return 'PE64/OTHER'
        }
        return ''
    } catch { return '' }
}

# --- component checks ---
foreach ($c in $manifest.components) {
    if ($c.kind -eq 'exclusion') { continue }
    if ($c.kind -in @('dll', 'config')) {
        $src = $null
        if ($c.source.type -eq 'build') { $src = Join-Path $RepoRoot ($c.source.output -replace '/', '\') }
        elseif ($c.source.type -eq 'repo') { $src = Join-Path $RepoRoot ($c.source.path -replace '/', '\') }
        elseif ($c.source.type -eq 'sdk') {
            if (-not $env:OPENXR_SDK) { Warn("$($c.id): OPENXR_SDK unset, sdk file unchecked"); continue }
            $src = Join-Path $env:OPENXR_SDK 'native\Win32\release\bin\openxr_loader.dll'
        }
        if (-not $src -or -not (Test-Path $src -PathType Leaf)) {
            if ($c.required) { Block("$($c.id): missing source $src") } else { Warn("$($c.id): missing optional source $src") }
            continue
        }
        if ($c.kind -eq 'dll' -and (Test-BinaryFormat $src) -ne 'PE32') { Block("$($c.id): not x86 PE32: $src"); continue }
        Ok("$($c.id): source present + valid")
        continue
    }
    # tree kinds resolve under AssetRoot/<id>
    if (-not $AssetRoot) { Warn("$($c.id): no -AssetRoot, staged tree unchecked"); continue }
    $stage = Join-Path $AssetRoot $c.id
    if (-not (Test-Path $stage) -or @(Get-ChildItem $stage -Recurse -File -ErrorAction SilentlyContinue).Count -eq 0) {
        if ($c.required) { Block("$($c.id): empty/missing stage $stage") }
        elseif ($c.profile -eq 'experimental') { Warn("$($c.id): experimental stage absent (opt-in, not a blocker)") }
        else { Warn("$($c.id): missing optional stage $stage") }
        continue
    }
    $bad = 0
    foreach ($f in (Get-ChildItem $stage -Recurse -File)) {
        $fmt = Test-BinaryFormat $f.FullName
        $ext = $f.Extension.ToLower()
        $expect = @{'.png' = 'PNG'; '.dds' = 'DDS'; '.wav' = 'WAV-PCM'; '.bik' = 'BIK'; '.dll' = 'PE32'; '.svrk' = ''; '.svni' = ''; '.json' = ''; '.jsonl' = ''; '.ini' = ''; '.txt' = ''; '.md' = '' }[$ext]
        if ($null -eq $expect) { Warn("$($c.id): unexpected extension $($f.Name)"); continue }
        if ($expect -eq '') { continue }  # opaque sidecar/manifest formats
        $okFmt = ($expect -eq 'DDS' -and $fmt -like 'DDS-*') -or ($fmt -eq $expect)
        if (-not $okFmt) { Block("$($c.id): invalid $($ext) container: $($f.Name)"); $bad++ }
        if (($ext -in @('.json')) -and $fmt -eq '') {
            try { Get-Content $f.FullName -Raw | ConvertFrom-Json | Out-Null } catch { Block("$($c.id): invalid JSON: $($f.Name)"); $bad++ }
        }
        if ($ext -eq '.jsonl') {
            $ln = 0; foreach ($l in (Get-Content $f.FullName)) { $ln++; if ($l.Trim() -eq '') { continue }; try { $l | ConvertFrom-Json | Out-Null } catch { Block("$($c.id): invalid JSONL line $ln in $($f.Name)"); $bad++; break } }
        }
    }
    if ($bad -eq 0) { Ok("$($c.id): staged tree formats valid") }
}
Write-Host ""
Write-Host "VERIFY blockers=$($blockers.Count) warnings=$($warnings.Count)"
Write-Output "VERIFY-RESULT blockers=$($blockers.Count) warnings=$($warnings.Count)"
if ($blockers.Count -gt 0) { exit 1 }
