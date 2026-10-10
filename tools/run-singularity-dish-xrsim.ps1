# Dishonored-VR style xrsim test on the Steam Singularity install (not the project's steam-xrsim runner).
# Direct exe launch from Steam Binaries with XR_RUNTIME_JSON (Singularity has no [VR] XrRuntimeJson ini).
param(
    [string]$SteamRoot = 'C:\Program Files (x86)\Steam',
    [string]$SteamBin = '',
    [string]$Sequence = '',
    [int]$WaitSessionSec = 120,
    [string]$VrRec = '',
    [switch]$NoSteamRestart,
    [switch]$KeyboardOnly
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$dish = 'C:\Users\Trilex\vr-mods\references\Dishonored-VR\tools'
if (-not $SteamBin) { $SteamBin = Join-Path $SteamRoot 'steamapps\common\Singularity\Binaries' }
if (-not $Sequence) { $Sequence = Join-Path $repo 'tools\xrsim\singularity-steam-gameplay.xrs' }
if (-not $VrRec) {
    $VrRec = Join-Path $env:LOCALAPPDATA 'SingularityVR\vr_sessions\session_2026-10-09_14-18-07.vrrec'
}
$replayScript = Join-Path $repo 'tools\xrsim_replay_vr_session.py'
$exe = Join-Path $SteamBin 'Singularity.exe'
$steamExe = Join-Path $SteamRoot 'steam.exe'
$logRoot = Join-Path $env:LOCALAPPDATA 'SingularityVR'
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$xrsimDir = Join-Path $logRoot "xrsim-steam-$stamp"
$evidence = Join-Path $repo "deployment\agent-verify-20261010\evidence\dish-style-$stamp"
New-Item -ItemType Directory -Force -Path $evidence, $xrsimDir | Out-Null

function Write-Step($m) { Write-Host ("[{0}] {1}" -f (Get-Date -Format 'HH:mm:ss'), $m) }

if (-not (Test-Path -LiteralPath $exe)) { throw "Singularity.exe not found: $exe" }
$dll = Join-Path $SteamBin 'd3d9.dll'
$dllHash = (Get-FileHash -LiteralPath $dll -Algorithm SHA256).Hash.ToUpperInvariant()
Write-Step "Steam Binaries d3d9.dll SHA-256: $dllHash"

Get-Process -Name Singularity -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 2

Write-Step "xrsim-install -> $xrsimDir"
& (Join-Path $dish 'xrsim-install.ps1') -Release -Dir $xrsimDir | Out-Null
$manifest = Join-Path $xrsimDir 'dvr_xrsim32.json'

$savedRt = $env:XR_RUNTIME_JSON
$savedDir = $env:DVR_XRSIM_DIR
$env:XR_RUNTIME_JSON = $manifest
$env:DVR_XRSIM_DIR = $xrsimDir
try {
    # Singularity needs Steam DRM (direct exe often exits immediately — see STEAM_XRSIM_RUNNER_20261008.md).
    if (-not (Test-Path -LiteralPath $steamExe)) { throw "steam.exe not found: $steamExe" }
    $startedAt = Get-Date
    if (-not $NoSteamRestart -and (Get-Process -Name steam -ErrorAction SilentlyContinue)) {
        Write-Step 'Restarting Steam so applaunch inherits XR_RUNTIME_JSON (Dishonored xrsim-launch -ViaSteam pattern)'
        Start-Process -FilePath $steamExe -ArgumentList @('-shutdown') -Wait | Out-Null
        $sd = (Get-Date).AddSeconds(25)
        while ((Get-Date) -lt $sd -and (Get-Process -Name steam -ErrorAction SilentlyContinue)) { Start-Sleep -Milliseconds 300 }
        Start-Sleep -Seconds 2
    }
    if (-not (Get-Process -Name steam -ErrorAction SilentlyContinue)) {
        # Do not use -silent: it has blocked applaunch in prior headset runs.
        $boot = Start-Process -FilePath $steamExe -WorkingDirectory $SteamRoot -PassThru
        Write-Step "Started Steam PID $($boot.Id); waiting for client"
        Start-Sleep -Seconds 12
    }
    Write-Step 'steam -applaunch 42670 -windowed -ResX=1280 -ResY=720'
    Start-Process -FilePath $steamExe -WorkingDirectory $SteamRoot `
        -ArgumentList @('-applaunch', '42670', '-windowed', '-ResX=1280', '-ResY=720') | Out-Null
    $deadline = (Get-Date).AddSeconds($WaitSessionSec)
    $ready = $false
    $p = $null
    $st = $null
    while ((Get-Date) -lt $deadline) {
        $candidates = @(Get-Process -Name Singularity -ErrorAction SilentlyContinue |
            Where-Object { $_.StartTime -ge $startedAt.AddSeconds(-3) })
        if ($candidates.Count) { $p = $candidates | Sort-Object StartTime -Descending | Select-Object -First 1 }
        $stPath = Join-Path $xrsimDir 'state.json'
        if ($p -and (Test-Path -LiteralPath $stPath)) {
            $st = Get-Content -LiteralPath $stPath -Raw | ConvertFrom-Json
            if ([int]$st.pid -eq $p.Id -and $st.sessionRunning -and $st.sessionState -in @('FOCUSED', 'VISIBLE') -and [int]$st.projectionViews -ge 2) {
                $ready = $true
                break
            }
        }
        Start-Sleep -Milliseconds 500
    }
    if (-not $p) { throw "Singularity process did not appear within ${WaitSessionSec}s" }
    if (-not $ready) { throw "xrsim session not ready for PID $($p.Id) within ${WaitSessionSec}s" }
    Write-Step "xrsim ready pid=$($p.Id) frame=$($st.frame)"

    $shot = Join-Path $dish 'xrsim-shot.ps1'
    $shots = @()
    if (-not $KeyboardOnly -and (Test-Path -LiteralPath $VrRec) -and (Test-Path -LiteralPath $replayScript)) {
        Write-Step "Canonical .vrrec replay (Dishonored playbook / STEAM_XRSIM_RUNNER_PLAYBOOK)"
        Start-Sleep -Seconds 6
        $replayReport = Join-Path $evidence 'vrrec-replay.json'
        & python $replayScript $VrRec --dir $xrsimDir --stride 4 --speed 8 --report $replayReport
        if ($LASTEXITCODE -ne 0) { throw "vrrec replay failed with exit $LASTEXITCODE" }
        Start-Sleep -Seconds 8
        $outBase = Join-Path $evidence 'steam-gameplay-hands-shadows'
        $s = & $shot -Dir $xrsimDir -Out $outBase -Quiet
        $shots += $s
        Write-Step ("shot steam-gameplay-hands-shadows: L={0} R={1}" -f $s.Left, $s.Right)
    }

    # Optional keyboard .xrs (splash skip); skipped when vrrec path succeeded above unless -KeyboardOnly.
    if ($KeyboardOnly -or -not $shots.Count) {
    $cmd = Join-Path $dish 'xrsim-cmd.ps1'
    $state = Join-Path $dish 'xrsim-state.ps1'
    $key = Join-Path $repo 'tools\singularity-game-key.ps1'
    $steps = Get-Content -LiteralPath $Sequence
    foreach ($line in $steps) {
        $t = $line.Trim()
        if (-not $t -or $t.StartsWith('#')) { continue }
        if ($t -eq 'reset') { & $cmd -Dir $xrsimDir -Quiet 'reset' | Out-Null; continue }
        if ($t -match '^@frames\s+(\d+)') {
            & $state -Dir $xrsimDir -For ("frame+$($Matches[1])") -TimeoutSec 30 -Quiet | Out-Null
            continue
        }
        if ($t -match '^@assert\s+(\S+)\s+(\S+)\s+(.+)$') {
            $keyName = $Matches[1]; $op = $Matches[2]; $want = $Matches[3]
            $s = & $state -Dir $xrsimDir -Quiet
            $have = $s.$keyName
            $ok = switch ($op) {
                'eq' { "$have" -eq $want }
                'ge' { [double]$have -ge [double]$want }
                default { throw "unsupported assert op $op" }
            }
            if (-not $ok) { throw "assert failed: $keyName $op $want (have $have)" }
            continue
        }
        if ($t -match '^@key\s+(\S+)(?:\s+(\d+))?(?:\s+(\d+))?$') {
            $k = $Matches[1]; $rep = if ($Matches[2]) { [int]$Matches[2] } else { 1 }
            $delay = if ($Matches[3]) { [int]$Matches[3] } else { 500 }
            & $key -Key $k -Repeat $rep -Delay $delay
            continue
        }
        if ($t -match '^@xr\s+(.+)$') {
            & $cmd -Dir $xrsimDir -Quiet $Matches[1]
            continue
        }
        if ($t -match '^@shot\s+(\S+)') {
            $name = $Matches[1]
            $outBase = Join-Path $evidence $name
            $s = & $shot -Dir $xrsimDir -Out $outBase -Quiet
            $shots += $s
            Write-Step ("shot {0}: L={1} R={2}" -f $name, $s.Left, $s.Right)
            continue
        }
        throw "unknown step: $t"
    }
    }

    $log = Get-ChildItem -LiteralPath $logRoot -Filter 'view_matrix*.log' -File | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if ($log) {
        Copy-Item -LiteralPath $log.FullName -Destination (Join-Path $evidence 'view_matrix.log') -Force
        $patterns = 'ShadowMode|shadow-sample|ShadowMode3|GloveIsolation|GloveFaceFilter|suppressed detached|IndependentHandsAudit|gameplay'
        Select-String -LiteralPath $log.FullName -Pattern $patterns | Select-Object -Last 40 |
            ForEach-Object { $_.Line } | Set-Content (Join-Path $evidence 'log-tail.txt') -Encoding UTF8
    }
    Copy-Item -LiteralPath (Join-Path $xrsimDir 'state.json') -Destination (Join-Path $evidence 'xrsim-state.json') -Force

    [ordered]@{
        evidenceDir = $evidence
        dllSha256 = $dllHash
        gamePid = $p.Id
        xrsimDir = $xrsimDir
        shots = @($shots | ForEach-Object { @{ Left = $_.Left; Right = $_.Right; Sbs = $_.Sbs } })
    } | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $evidence 'manifest.json') -Encoding UTF8
    Write-Step "Done. Evidence: $evidence"
} finally {
    $env:XR_RUNTIME_JSON = $savedRt
    $env:DVR_XRSIM_DIR = $savedDir
    Get-Process -Name Singularity -ErrorAction SilentlyContinue | Stop-Process -Force
    # Steam keeps XR_RUNTIME_JSON from the test launch; restart clean so the next headset run uses SteamVR.
    $restore = Join-Path $repo 'tools\Restore-Singularity-HeadsetLaunch.ps1'
    if (Test-Path -LiteralPath $restore) {
        Write-Step 'Restoring clean Steam (no xrsim env) for normal PSVR2/SteamVR play'
        & $restore
    }
}
