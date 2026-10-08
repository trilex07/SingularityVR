[CmdletBinding(SupportsShouldProcess)]
param(
    [ValidateSet('vfx','shadow1','shadow3','vfx-shadow3','desktop-hmd','desktop-hmd-shadow1')]
    [string]$Candidate = 'vfx',
    [string]$SteamRoot = 'C:\Program Files (x86)\Steam',
    [string]$RuntimeManifest = 'C:\Users\Trilex\vr-mods\xrsim-scratch-run\dvr_xrsim32.json',
    [int]$WaitSeconds = 90,
    [switch]$StopSteamVR,
    [switch]$LaunchViaSteam,
    [switch]$RestartSteam,
    [switch]$KeepCandidate,
    [switch]$SkipLaunch,
    [switch]$NoRestore,
    [string]$ReportDirectory = 'C:\Users\Trilex\vr-mods\deployment\visual-audit-20261008'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$candidateDir = switch ($Candidate) {
    'vfx'        { Join-Path $repoRoot 'deployment\vfx-stereo-eye-fix-20261008' }
    'shadow1'    { Join-Path $repoRoot 'deployment\shadow1-ab-20261008' }
    'shadow3'    { Join-Path $repoRoot 'deployment\shadow-mode3-ab-20261008' }
    'vfx-shadow3' { Join-Path $repoRoot 'deployment\vfx-shadow3-latest-20261008' }
    'desktop-hmd' { Join-Path $repoRoot 'deployment\desktop-hmd-diagnostic-20261008' }
    'desktop-hmd-shadow1' { Join-Path $repoRoot 'deployment\desktop-hmd-shadow1-20261008' }
}
$gameDir = Join-Path $SteamRoot 'steamapps\common\Singularity\Binaries'
$gameExe = Join-Path $gameDir 'Singularity.exe'
$dllPath = Join-Path $gameDir 'd3d9.dll'
$iniPath = Join-Path $gameDir 'SingularityVR.ini'
$vfxPath = Join-Path $gameDir 'SingularityVfx.dll'
$runStamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$backupDir = Join-Path $candidateDir "runner-backup-$runStamp"
$reportPath = Join-Path $ReportDirectory "steam-xrsim-run-$runStamp.json"
$logRoot = Join-Path $env:LOCALAPPDATA 'SingularityVR'

function Write-Runner([string]$Message) {
    Write-Host ("[{0}] {1}" -f (Get-Date -Format 'HH:mm:ss'), $Message)
}

function Stop-ExactProcesses([string[]]$Names) {
    foreach ($name in $Names) {
        $procs = @(Get-Process -Name $name -ErrorAction SilentlyContinue)
        foreach ($proc in $procs) {
            Write-Runner "Stopping $($proc.ProcessName) PID $($proc.Id) to release files."
            Stop-Process -Id $proc.Id -Force -ErrorAction Stop
        }
    }
    Start-Sleep -Milliseconds 500
}

function Test-FileFree([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path)) { return $true }
    try {
        $stream = [System.IO.File]::Open($Path, [System.IO.FileMode]::Open,
            [System.IO.FileAccess]::ReadWrite, [System.IO.FileShare]::None)
        $stream.Dispose()
        return $true
    } catch {
        return $false
    }
}

function Get-LockEvidence([string]$Path) {
    $result = @()
    $openfiles = Join-Path $env:WINDIR 'System32\openfiles.exe'
    if (Test-Path -LiteralPath $openfiles) {
        try {
            $raw = & $openfiles /query /fo csv /v 2>$null
            foreach ($line in @($raw)) {
                if ($line -and $line -match [regex]::Escape($Path)) { $result += [string]$line }
            }
        } catch { }
    }
    return @($result)
}

function Copy-Unlocked([string]$Source, [string]$Destination, [int]$Attempts = 12) {
    if (-not (Test-Path -LiteralPath $Source)) { throw "Source file not found: $Source" }
    for ($i = 1; $i -le $Attempts; $i++) {
        if (Test-FileFree $Destination) {
            try {
                Copy-Item -LiteralPath $Source -Destination $Destination -Force -ErrorAction Stop
                $srcHash = (Get-FileHash -LiteralPath $Source -Algorithm SHA256).Hash
                $dstHash = (Get-FileHash -LiteralPath $Destination -Algorithm SHA256).Hash
                if ($srcHash -ne $dstHash) { throw "Copy hash differs from candidate." }
                return
            } catch {
                if ($i -eq $Attempts) { throw }
            }
        }
        Start-Sleep -Milliseconds 350
    }
    # A loaded DLL may deny overwrite while still allowing an atomic rename.
    # Quarantine the old file, copy the new image, then remove the quarantine.
    $quarantine = "$Destination.locked-$PID-$([guid]::NewGuid().ToString('N'))"
    try {
        Rename-Item -LiteralPath $Destination -NewName (Split-Path -Leaf $quarantine) -Force -ErrorAction Stop
        Copy-Item -LiteralPath $Source -Destination $Destination -Force -ErrorAction Stop
        $srcHash = (Get-FileHash -LiteralPath $Source -Algorithm SHA256).Hash
        $dstHash = (Get-FileHash -LiteralPath $Destination -Algorithm SHA256).Hash
        if ($srcHash -ne $dstHash) { throw 'Atomic replacement hash differs from candidate.' }
        Remove-Item -LiteralPath $quarantine -Force -ErrorAction SilentlyContinue
        return
    } catch {
        if ((Test-Path -LiteralPath $quarantine) -and -not (Test-Path -LiteralPath $Destination)) {
            Rename-Item -LiteralPath $quarantine -NewName (Split-Path -Leaf $Destination) -Force -ErrorAction SilentlyContinue
        }
    }
    $owners = @(Get-LockEvidence $Destination)
    $detail = if ($owners.Count) { ($owners -join "`n") } else { 'openfiles did not report an owner.' }
    throw "File is still locked: $Destination`n$detail"
}

function Save-Backup {
    New-Item -ItemType Directory -Force -Path $backupDir | Out-Null
    $manifest = [ordered]@{}
    foreach ($path in @($dllPath,$iniPath,$vfxPath)) {
        $name = Split-Path -Leaf $path
        $manifest[$name] = Test-Path -LiteralPath $path
        if ($manifest[$name]) { Copy-Unlocked $path (Join-Path $backupDir $name) }
    }
    $manifest | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $backupDir 'manifest.json') -Encoding UTF8
}

function Restore-Backup {
    $manifestPath = Join-Path $backupDir 'manifest.json'
    if (-not (Test-Path -LiteralPath $manifestPath)) { return }
    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    Stop-ExactProcesses @('Singularity','SingularityVR')
    foreach ($name in @('d3d9.dll','SingularityVR.ini','SingularityVfx.dll')) {
        $present = [bool]$manifest.$name
        $dst = Join-Path $gameDir $name
        $src = Join-Path $backupDir $name
        if ($present) { Copy-Unlocked $src $dst }
        elseif (Test-Path -LiteralPath $dst) { Remove-Item -LiteralPath $dst -Force }
    }
    Write-Runner "Restored previous Steam state from $backupDir"
}

function Get-NewestLog([datetime]$Since) {
    if (-not (Test-Path -LiteralPath $logRoot)) { return $null }
    $logs = @(Get-ChildItem -LiteralPath $logRoot -Filter 'view_matrix_*.log' -File -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending)
    foreach ($log in $logs) {
        if ($log.LastWriteTime -ge $Since.AddSeconds(-2)) { return $log }
    }
    return $null
}

function Get-BestRunLog([datetime]$Since) {
    if (-not (Test-Path -LiteralPath $logRoot)) { return $null }
    $logs = @(Get-ChildItem -LiteralPath $logRoot -Filter 'view_matrix_*.log' -File -ErrorAction SilentlyContinue |
        Where-Object { $_.LastWriteTime -ge $Since.AddSeconds(-2) } |
        Sort-Object -Property @{Expression='Length';Descending=$true}, @{Expression='LastWriteTime';Descending=$true})
    foreach ($log in $logs) {
        $head = Get-Content -LiteralPath $log.FullName -Raw -ErrorAction SilentlyContinue
        if ($head -match 'Direct3DCreate9Ex|D3D9Ex device created|input-device cache ACTIVE') { return $log }
    }
    return ($logs | Select-Object -First 1)
}

function Read-RunEvidence([string]$LogPath) {
    $text = if ($LogPath -and (Test-Path -LiteralPath $LogPath)) { Get-Content -LiteralPath $LogPath -Raw } else { '' }
    $statePath = Join-Path $env:USERPROFILE 'xrsim-state.json'
    $state = Join-Path (Split-Path -Parent $RuntimeManifest) 'state.json'
    [ordered]@{
        log_path = $LogPath
        runtime_manifest = $RuntimeManifest
        runtime_ready = ($text -match 'dvr-xrsim' -and $text -match 'READY|FOCUSED|pipeline')
        missing_mouse_path = ($text -match 'mouse|DirectInput' -and $text -match 'compat|skip|null')
        foreground_boundary = ($text -match 'foreground|FgMatch|boundary')
        shadow_path = ($text -match 'shadow')
        vfx_path = ($text -match 'VFX|muzzle|screen flash')
        log_tail = (($text -split "`r?`n" | Select-Object -Last 80) -join "`n")
        xrsim_state = if (Test-Path -LiteralPath $state) { Get-Content -LiteralPath $state -Raw } else { $null }
        user_state = if (Test-Path -LiteralPath $statePath) { Get-Content -LiteralPath $statePath -Raw } else { $null }
    }
}

function Write-ReportFile {
    try {
        $safe = [ordered]@{
            timestamp = [string]$report.timestamp
            finished = [string]$report.finished
            candidate = [string]$report.candidate
            candidate_dir = [string]$report.candidate_dir
            game_exe = [string]$report.game_exe
            dll_before = [string]$report.dll_before
            dll_candidate = [string]$report.dll_candidate
            dll_after_install = [string]$report.dll_after_install
            launched = [bool]$report.launched
            restored = [bool]$report.restored
            error = [string]$report.error
            restore_error = [string]$report.restore_error
            log_path = if ($report.evidence -and $report.evidence.log_path) { [string]$report.evidence.log_path } else { $null }
            runtime_ready = if ($report.evidence) { [bool]$report.evidence.runtime_ready } else { $false }
            missing_mouse_path = if ($report.evidence) { [bool]$report.evidence.missing_mouse_path } else { $false }
            foreground_boundary = if ($report.evidence) { [bool]$report.evidence.foreground_boundary } else { $false }
            shadow_path = if ($report.evidence) { [bool]$report.evidence.shadow_path } else { $false }
            vfx_path = if ($report.evidence) { [bool]$report.evidence.vfx_path } else { $false }
            log_tail = if ($report.evidence -and $report.evidence.log_tail) { [string]$report.evidence.log_tail } else { '' }
        }
        $json = $safe | ConvertTo-Json -Depth 4
        [System.IO.File]::WriteAllText($reportPath, $json, (New-Object System.Text.UTF8Encoding($false)))
        Write-Runner "Report: $reportPath"
    } catch {
        $fallback = "report serialization failed: $($_.Exception.Message)`n$($report | Out-String)"
        [System.IO.File]::WriteAllText(($reportPath + '.txt'), $fallback, (New-Object System.Text.UTF8Encoding($false)))
        Write-Runner "Report serialization error; fallback: $reportPath.txt"
    }
}

if (-not (Test-Path -LiteralPath $gameExe)) { throw "Singularity.exe not found: $gameExe" }
if (-not (Test-Path -LiteralPath (Join-Path $candidateDir 'd3d9.dll'))) { throw "Brak DLL kandydata: $candidateDir" }
if (-not (Test-Path -LiteralPath $RuntimeManifest)) { throw "Brak manifestu OpenXR simulatora: $RuntimeManifest" }
New-Item -ItemType Directory -Force -Path $ReportDirectory | Out-Null

$startedAt = Get-Date
$oldRuntime = $env:XR_RUNTIME_JSON
$oldXrsimDir = $env:DVR_XRSIM_DIR
$gameProcess = $null
$log = $null
$report = [ordered]@{
    timestamp = $startedAt.ToString('o')
    candidate = $Candidate
    candidate_dir = $candidateDir
    game_exe = $gameExe
    dll_before = if (Test-Path -LiteralPath $dllPath) { (Get-FileHash $dllPath -Algorithm SHA256).Hash } else { $null }
    dll_candidate = (Get-FileHash (Join-Path $candidateDir 'd3d9.dll') -Algorithm SHA256).Hash
    launched = $false
    restored = $false
    error = $null
    restore_error = $null
    dll_after_install = $null
    evidence = $null
    process_exit = $false
    exit_code = $null
    finished = $null
}

try {
    Stop-ExactProcesses @('Singularity','SingularityVR')
    if ($StopSteamVR) { Stop-ExactProcesses @('vrserver','vrcompositor','vrdashboard','steamvr') }
    Save-Backup
    Copy-Unlocked (Join-Path $candidateDir 'd3d9.dll') $dllPath
    Copy-Unlocked (Join-Path $candidateDir 'SingularityVR.ini') $iniPath
    $candidateVfx = Join-Path $candidateDir 'SingularityVfx.dll'
    if (Test-Path -LiteralPath $candidateVfx) { Copy-Unlocked $candidateVfx $vfxPath }
    elseif (Test-Path -LiteralPath $vfxPath) { Remove-Item -LiteralPath $vfxPath -Force }
    $report.dll_after_install = (Get-FileHash $dllPath -Algorithm SHA256).Hash
    Write-Runner "Installed candidate $Candidate ($($report.dll_after_install))."

    if (-not $SkipLaunch) {
        $env:XR_RUNTIME_JSON = $RuntimeManifest
        $env:DVR_XRSIM_DIR = Split-Path -Parent $RuntimeManifest
        $logBefore = Get-NewestLog $startedAt
        if ($LaunchViaSteam) {
            $steamExe = Join-Path $SteamRoot 'steam.exe'
            if (-not (Test-Path -LiteralPath $steamExe)) { throw "Steam executable not found: $steamExe" }
            if ($RestartSteam) {
                Write-Runner 'RestartSteam requested: asking the Steam client to shut down before applaunch.'
                $shutdown = Start-Process -FilePath $steamExe -ArgumentList @('-shutdown') -PassThru -Wait
                Start-Sleep -Seconds 5
            }
            if (-not (Get-Process -Name steam -ErrorAction SilentlyContinue)) {
                $steamBoot = Start-Process -FilePath $steamExe -WorkingDirectory $SteamRoot -ArgumentList @('-silent') -PassThru
                Write-Runner "Started Steam client PID $($steamBoot.Id); waiting for app registry."
                Start-Sleep -Seconds 12
            }
            $gameProcess = Start-Process -FilePath $steamExe -WorkingDirectory $SteamRoot -ArgumentList @('-applaunch','42670') -PassThru
            Write-Runner "Started Steam applaunch 42670 (launcher PID $($gameProcess.Id))."
        } else {
            $gameProcess = Start-Process -FilePath $gameExe -WorkingDirectory $gameDir -ArgumentList @('-windowed','-ResX=1280','-ResY=720') -PassThru
            Write-Runner "Started Singularity PID $($gameProcess.Id) with dvr-xrsim."
        }
        $report.launched = $true
        $deadline = (Get-Date).AddSeconds($WaitSeconds)
        do {
            $log = Get-NewestLog $startedAt
            if ($log) {
                $tail = Get-Content -LiteralPath $log.FullName -Raw -ErrorAction SilentlyContinue
                # The first DLL attach log can be only a few hundred bytes.  Wait for
                # the D3D9 device marker so a later full log from the restarted game
                # is not mistaken for the completed run.
                if ($tail -match 'Direct3DCreate9Ex|D3D9Ex device created|input-device cache ACTIVE') { break }
            }
            Start-Sleep -Milliseconds 500
        } while ((Get-Date) -lt $deadline)
        $log = Get-BestRunLog $startedAt
        $report.evidence = Read-RunEvidence $(if ($log) { $log.FullName } else { $null })
        $report.process_exit = $false
        if ($gameProcess.HasExited) { $report.process_exit = $true; $report.exit_code = $gameProcess.ExitCode }
        Write-Runner "Collected log: $($(if ($log) { $log.FullName } else { 'no new log' }))"
    }
} catch {
    $report.error = $_.Exception.Message
    Write-Runner "ERROR: $($report.error)"
    throw
} finally {
    Write-Runner "Cleanup begin"
    $env:XR_RUNTIME_JSON = $oldRuntime
    $env:DVR_XRSIM_DIR = $oldXrsimDir
    $report.finished = (Get-Date).ToString('o')
    Write-ReportFile
    Write-Runner "Cleanup report written"
    if (-not $KeepCandidate -and -not $NoRestore -and (Test-Path -LiteralPath $backupDir)) {
        try { Restore-Backup; $report.restored = $true }
        catch { $report.restore_error = $_.Exception.Message; Write-Runner "Restore error: $($_.Exception.Message)" }
    }
    Write-Runner "Cleanup restore complete"
    $report.finished = (Get-Date).ToString('o')
    Write-ReportFile
}

