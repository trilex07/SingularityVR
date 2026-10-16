<#
Install-SingularityVR.ps1 - manifest-driven offline installer (SIDE, no Steam writes outside -GamePath).

  Install:   .\Install-SingularityVR.ps1 -GamePath <dir> -Profile Core [,Localization] [-AssetRoot <dir>] [-AllowExperimental]
  Repair:    ... -Mode Repair
  Uninstall: ... -Mode Uninstall

Rules: fail closed on unknown game build (unless -ForceUnknownGameBuild),
backup-everything-it-touches, INI merge preserves user keys, rollback on any
failure with verification, never deletes unknown files, no admin required.
#>
[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [Parameter(Mandatory = $true)][string]$GamePath,
    [ValidateSet('Install', 'Repair', 'Uninstall')][string]$Mode = 'Install',
    [string[]]$Profile = @('Core'),
    [string]$AssetRoot = '',
    [string]$ManifestPath = '',
    [string]$BackupRoot = '',
    [switch]$AllowExperimental,
    [switch]$ForceUnknownGameBuild
)
$ErrorActionPreference = 'Stop'

$here = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $ManifestPath) { $ManifestPath = Join-Path $here 'release_manifest.json' }
$manifest = Get-Content $ManifestPath -Raw | ConvertFrom-Json
$RepoRoot = Resolve-Path (Join-Path $here '..\..')
if (-not $BackupRoot) { $BackupRoot = Join-Path $GamePath 'SingularityVR-backups' }

function Log($m) { Write-Host $m }
function Fail($m) { throw "INSTALLER-FAIL: $m" }

function Get-Sha256([string]$p) {
    return (Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.ToUpperInvariant()
}
function Assert-Inside([string]$root, [string]$p) {
    $r = [IO.Path]::GetFullPath($root); $f = [IO.Path]::GetFullPath($p)
    if ($f -ne $r -and -not $f.StartsWith($r + [IO.Path]::DirectorySeparatorChar)) { Fail("path escape: $p") }
}
function Get-Relative([string]$root, [string]$p) {
    # .NET Framework (WinPS 5.1) has no Path.GetRelativePath; prefix math suffices here.
    $sep = [IO.Path]::DirectorySeparatorChar
    $r = [IO.Path]::GetFullPath($root).TrimEnd($sep) + $sep
    $f = [IO.Path]::GetFullPath($p)
    if (-not $f.StartsWith($r, [StringComparison]::OrdinalIgnoreCase)) { Fail("path escape: $p") }
    return $f.Substring($r.Length)
}
function Read-Ini([string]$p) {
    $secs = [ordered]@{}; $cur = ''
    if (Test-Path $p) { foreach ($l in (Get-Content $p)) {
        if ($l -match '^\s*\[(.+?)\]\s*$') { $cur = $Matches[1]; if (-not $secs.Contains($cur)) { $secs[$cur] = [ordered]@{} } }
        elseif ($l -match '^\s*([^=;#]+?)\s*=\s*(.*?)\s*$' -and $cur -ne '') { $k = $Matches[1].Trim(); if (-not $secs[$cur].Contains($k)) { $secs[$cur][$k] = $Matches[2] } }
    } }
    return $secs
}
function Merge-Ini([string]$dst, [string]$src) {
    # Add keys missing in dst from src; never overwrite user values.
    $d = Read-Ini $dst; $s = Read-Ini $src; $added = 0
    $out = @(); $cur = ''
    if (Test-Path $dst) { $out += Get-Content $dst }
    foreach ($sec in $s.Keys) {
        if (-not $d.Contains($sec)) { $out += ''; $out += "[$sec]"; $d[$sec] = [ordered]@{} }
        foreach ($k in $s[$sec].Keys) {
            if (-not $d[$sec].Contains($k)) { $out += "$k=$($s[$sec][$k])"; $added++ }
        }
    }
    $enc = New-Object Text.UTF8Encoding($false)
    [IO.File]::WriteAllLines($dst, $out, $enc)
    return $added
}

# ---- resolve game dir ----
$GamePath = [IO.Path]::GetFullPath($GamePath)
if (-not (Test-Path $GamePath -PathType Container)) { Fail("game dir missing: $GamePath") }
$exe = Join-Path $GamePath $manifest.game.exe
if (-not (Test-Path $exe -PathType Leaf)) { Fail("not a Singularity install (missing $($manifest.game.exe))") }
$exeHash = Get-Sha256 $exe
if ($manifest.game.builds.Count -eq 0 -and -not $ForceUnknownGameBuild) {
    Fail("no pinned game builds in manifest; re-run with -ForceUnknownGameBuild to proceed explicitly")
}
if ($manifest.game.builds.Count -gt 0 -and -not ($manifest.game.builds -contains $exeHash)) {
    Fail("unsupported game build $exeHash")
}
Log("game OK: $exe")

$installedPath = Join-Path $GamePath 'SingularityVR-installed.json'
$wanted = @()
foreach ($c in $manifest.components) {
    if ($c.kind -eq 'exclusion') { continue }
    if ($Profile -notcontains $c.profile) { continue }
    if ($c.profile -eq 'experimental' -and -not $AllowExperimental) { continue }
    if ($c.id -eq 'texture-tier') { continue }  # tier content selected via texture_override.ini; files deploy with injector profile
    $wanted += $c
}
if ($Mode -eq 'Uninstall') {
    if (-not (Test-Path $installedPath)) { Log("nothing installed (no manifest); nothing to do"); return }
    $inst = Get-Content $installedPath -Raw | ConvertFrom-Json
    $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
    foreach ($f in $inst.files) {
        $target = Join-Path $GamePath $f.dest
        if ($f.backup -and (Test-Path $f.backup)) {
            Copy-Item $f.backup $target -Force; Log("restored $($f.dest)")
        } elseif ($f.added -and (Test-Path $target)) {
            Remove-Item $target -Force; Log("removed $($f.dest)")
        }
    }
    # remove now-empty mod dirs (only known roots, only if empty)
    foreach ($d in @('Binaries\vr-assets')) {
        $p = Join-Path $GamePath $d
        if ((Test-Path $p) -and @(Get-ChildItem $p -Recurse -Force).Count -eq 0) { Remove-Item $p -Force }
    }
    Remove-Item $installedPath -Force
    Log("UNINSTALL complete"); return
}

# ---- plan ----
$plan = @()  # @{comp, src, dest, isDir}
foreach ($c in $wanted) {
    if ($c.kind -in @('dll', 'config')) {
        $src = $null
        if ($c.source.type -eq 'build') { $src = Join-Path $RepoRoot ($c.source.output -replace '/', '\') }
        elseif ($c.source.type -eq 'repo') { $src = Join-Path $RepoRoot ($c.source.path -replace '/', '\') }
        elseif ($c.source.type -eq 'sdk') {
            $sdk = $env:OPENXR_SDK
            if (-not $sdk) { Fail("OPENXR_SDK not set (needed for openxr_loader.dll)") }
            $src = Join-Path $sdk 'native\Win32\release\bin\openxr_loader.dll'
        }
        if (-not $src -or -not (Test-Path $src -PathType Leaf)) { Fail("missing source for $($c.id): $src") }
        $plan += [pscustomobject]@{comp = $c.id; src = $src; dest = ($c.dest -replace '/', '\'); isDir = $false }
    } else {
        if (-not $AssetRoot) { Fail("component $($c.id) needs -AssetRoot") }
        $stage = Join-Path $AssetRoot $c.id
        if (-not (Test-Path $stage) -or @(Get-ChildItem $stage -Recurse -File).Count -eq 0) { Fail("empty/missing stage for $($c.id): $stage") }
        $plan += [pscustomobject]@{comp = $c.id; src = $stage; dest = ($c.dest -replace '/', '\'); isDir = $true }
    }
}
function Install-TreeFile([string]$srcFile, [string]$dst, [string]$comp) {
    Assert-Inside $GamePath $dst
    New-Item -ItemType Directory -Force (Split-Path $dst) | Out-Null
    $existed = Test-Path $dst -PathType Leaf
    try { $s = [IO.File]::Open($dst, 'OpenOrCreate', 'ReadWrite', 'None'); $s.Close() }
    catch { Fail("file locked: $dst") }
    $rel = Get-Relative $GamePath $dst
    $rec = [ordered]@{comp = $comp; dest = $rel; backup = $null; added = $false; sha256 = $null }
    if ($prevRecs.Contains($rel) -and $prevRecs[$rel].backup) { $rec.backup = $prevRecs[$rel].backup }
    elseif ($prevRecs.Contains($rel) -and $prevRecs[$rel].added) { $rec.added = $true }
    elseif ($existed) {
        $b = Join-Path $backupDir (($rel -replace '[\\/]', '_') + '.bak')
        Copy-Item $dst $b -Force; $rec.backup = $b
    } else { $rec.added = $true }
    Copy-Item $srcFile $dst -Force
    $rec.sha256 = Get-Sha256 $dst
    return $rec
}
# arch check for staged DLLs
foreach ($p in $plan) {
    $files = if ($p.isDir) { Get-ChildItem $p.src -Recurse -File } else { Get-Item $p.src }
    foreach ($f in $files) {
        if ($f.Extension -ieq '.dll') {
            $b = [IO.File]::ReadAllBytes($f.FullName)
            $pe = [BitConverter]::ToInt32($b, 0x3c)
            $machine = [BitConverter]::ToUInt16($b, $pe + 4)
            if ($machine -ne 0x014c) { Fail("not x86 PE32: $($f.FullName)") }
        }
        if ($f.Extension -ieq '.bik' -and $f.Length -lt 1024) { Fail("suspiciously small .bik: $($f.FullName)") }
    }
}

# ---- transaction ----
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$backupDir = Join-Path $BackupRoot $stamp
New-Item -ItemType Directory -Force $backupDir | Out-Null
$records = @()
$addedDirs = @()
try {
    # Preserve ORIGINAL backups across reinstalls so uninstall returns to
    # pristine state (never back up our own previously installed files).
    $prevRecs = @{}
    if (Test-Path $installedPath -PathType Leaf) {
        try {
            foreach ($r in ((Get-Content $installedPath -Raw | ConvertFrom-Json).files)) { $prevRecs[$r.dest] = $r }
        } catch { $prevRecs = @{} }
    }
    foreach ($p in $plan) {
        if ($p.isDir) {
            $staged = @(Get-ChildItem $p.src -Recurse -File)
            $leaf = Split-Path $p.dest -Leaf
            if ([IO.Path]::HasExtension($leaf) -and $staged.Count -eq 1 -and @(Get-ChildItem $p.src -Recurse -Directory).Count -eq 0) {
                # single-file stage onto a file dest (e.g. a .bik): copy as the file itself
                $r = Install-TreeFile $staged[0].FullName (Join-Path $GamePath $p.dest) $p.comp
                $records += $r
                continue
            }
            foreach ($f in $staged) {
                $rel = Get-Relative $p.src $f.FullName
                $dst = Join-Path (Join-Path $GamePath $p.dest) $rel
                Assert-Inside $GamePath $dst
                New-Item -ItemType Directory -Force (Split-Path $dst) | Out-Null
                $existed = Test-Path $dst -PathType Leaf
                try { $s = [IO.File]::Open($dst, 'OpenOrCreate', 'ReadWrite', 'None'); $s.Close() }
                catch { Fail("file locked: $dst") }
                $rec = [ordered]@{comp = $p.comp; dest = Get-Relative $GamePath $dst; backup = $null; added = $false; sha256 = $null }
                if ($prevRecs.Contains($rec.dest) -and $prevRecs[$rec.dest].backup) { $rec.backup = $prevRecs[$rec.dest].backup }
                elseif ($prevRecs.Contains($rec.dest) -and $prevRecs[$rec.dest].added) { $rec.added = $true }
                elseif ($existed) {
                    $b = Join-Path $backupDir ($rec.dest -replace '[\\/]', '_')
                    Copy-Item $dst $b -Force; $rec.backup = $b
                } else { $rec.added = $true }
                Copy-Item $f.FullName $dst -Force
                $rec.sha256 = Get-Sha256 $dst
                $records += $rec
            }
        } else {
            $dst = Join-Path $GamePath $p.dest
            Assert-Inside $GamePath $dst
            $existed = Test-Path $dst -PathType Leaf
            try { $s = [IO.File]::Open($dst, 'OpenOrCreate', 'ReadWrite', 'None'); $s.Close() }
            catch { Fail("file locked: $dst") }
            $rec = [ordered]@{comp = $p.comp; dest = $p.dest; backup = $null; added = $false; sha256 = $null }
            if ($prevRecs.Contains($p.dest) -and $prevRecs[$p.dest].backup) { $rec.backup = $prevRecs[$p.dest].backup }
            elseif ($prevRecs.Contains($p.dest) -and $prevRecs[$p.dest].added) { $rec.added = $true }
            if ($p.comp -eq 'vr-ini' -and $existed) {
                if (-not $rec.backup -and -not $rec.added) {
                    $b = Join-Path $backupDir 'SingularityVR.ini.pre-install'
                    Copy-Item $dst $b -Force; $rec.backup = $b
                }
                $n = Merge-Ini $dst $p.src; Log("INI merge: $n defaults added, user keys kept")
            } else {
                if (-not $rec.backup -and -not $rec.added) {
                    if ($existed) {
                        $b = Join-Path $backupDir (($p.dest -replace '[\\/]', '_') + '.bak')
                        Copy-Item $dst $b -Force; $rec.backup = $b
                    } else { $rec.added = $true }
                }
                Copy-Item $p.src $dst -Force
            }
            $rec.sha256 = Get-Sha256 $dst
            $records += $rec
        }
    }
    $man = [ordered]@{manifest = 'SingularityVR-installed'; version = 1; time = $stamp; profile = $Profile; files = $records }
    $man | ConvertTo-Json -Depth 5 | Set-Content $installedPath
    Log("$($Mode.ToUpper()) complete: $($records.Count) files, manifest + backup $stamp")
} catch {
    Log("ERROR: $($_.Exception.Message) -- rolling back owned files")
    foreach ($r in $records) {
        $target = Join-Path $GamePath $r.dest
        if ($r.backup -and (Test-Path $r.backup)) { Copy-Item $r.backup $target -Force }
        elseif ($r.added -and (Test-Path $target)) { Remove-Item $target -Force }
    }
    # verify rollback
    $ok = $true
    foreach ($r in $records) {
        $target = Join-Path $GamePath $r.dest
        if ($r.backup) { if ((Get-Sha256 $target) -ne (Get-Sha256 $r.backup)) { $ok = $false } }
        elseif ($r.added -and (Test-Path $target)) { $ok = $false }
    }
    if (-not $ok) { Fail("rollback verification FAILED") }
    Log("rollback verified")
    throw
}
