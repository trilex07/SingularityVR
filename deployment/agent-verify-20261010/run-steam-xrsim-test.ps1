$ErrorActionPreference = 'Stop'
$prod = 'C:\Users\Trilex\vr-mods\SingularityVR-production'
$xrDir = 'C:\Users\Trilex\vr-mods\xrsim-scratch-run'
$vrrec = Join-Path $env:LOCALAPPDATA 'SingularityVR\vr_sessions\session_2026-10-09_14-18-07.vrrec'
$replayScript = Join-Path $prod 'tools\xrsim_replay_vr_session.py'
$reportDir = Join-Path $prod 'deployment\agent-verify-20261010\reports'
$evidenceDir = Join-Path $prod 'deployment\agent-verify-20261010\evidence'
$steamBin = 'C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries'
$dinput = Join-Path $steamBin 'dinput8.dll'
$dinputOff = Join-Path $steamBin 'dinput8.dll.agent-off'
$logRoot = Join-Path $env:LOCALAPPDATA 'SingularityVR'

New-Item -ItemType Directory -Force -Path $reportDir, $evidenceDir | Out-Null
if (-not (Test-Path -LiteralPath $vrrec)) { throw "Missing vrrec: $vrrec" }

$dinputWasDisabled = $false
if (Test-Path -LiteralPath $dinput) {
    Move-Item -LiteralPath $dinput -Destination $dinputOff -Force
    $dinputWasDisabled = $true
    Write-Host "Disabled texture injector (renamed dinput8.dll)."
}

$replayJob = Start-Job -ScriptBlock {
    param($xrDir, $vrrec, $replayScript, $reportDir, $evidenceDir)
    $deadline = (Get-Date).AddMinutes(10)
    while ((Get-Date) -lt $deadline) {
        $stPath = Join-Path $xrDir 'state.json'
        if (Test-Path -LiteralPath $stPath) {
            try {
                $st = Get-Content -LiteralPath $stPath -Raw | ConvertFrom-Json
                if ($st.sessionRunning -and $st.sessionState -in @('FOCUSED', 'VISIBLE') -and [int]$st.projectionViews -ge 2) {
                    Start-Sleep -Seconds 8
                    $out = Join-Path $reportDir 'replay.json'
                    & python $replayScript $vrrec --dir $xrDir --stride 4 --speed 8 --report $out
                    if ($LASTEXITCODE -ne 0) { throw "replay exit $LASTEXITCODE" }
                    Start-Sleep -Seconds 20
                    $shotScript = 'C:\Users\Trilex\vr-mods\references\Dishonored-VR\tools\xrsim-shot.ps1'
                    $shotBase = Join-Path $evidenceDir 'gameplay-gloves-shadows'
                    & $shotScript -Dir $xrDir -Out $shotBase -Quiet
                    return "replay+shot ok"
                }
            } catch { }
        }
        Start-Sleep -Seconds 2
    }
    throw 'Timed out waiting for focused xrsim session before replay.'
} -ArgumentList $xrDir, $vrrec, $replayScript, $reportDir, $evidenceDir

try {
    & (Join-Path $prod 'tools\steam-xrsim-singularity.ps1') `
        -CandidateDirectory (Join-Path $prod 'spikes\view_matrix') `
        -RuntimeManifest (Join-Path $xrDir 'dvr_xrsim32.json') `
        -LaunchViaSteam -RestartSteam -Windowed -WindowWidth 1280 -WindowHeight 720 `
        -WaitSeconds 120 -PostReadyHoldSeconds 150 `
        -ReportDirectory $reportDir `
        -LogRoot $logRoot
    $replayResult = Receive-Job -Job $replayJob -Wait -AutoRemoveJob
    Write-Host "Replay job: $replayResult"
} finally {
    if ($dinputWasDisabled -and (Test-Path -LiteralPath $dinputOff)) {
        Move-Item -LiteralPath $dinputOff -Destination $dinput -Force
        Write-Host 'Restored dinput8.dll.'
    }
}

$log = Get-ChildItem -LiteralPath $logRoot -Filter 'view_matrix*.log' -File |
    Sort-Object LastWriteTime -Descending | Select-Object -First 1
if ($log) {
    Copy-Item -LiteralPath $log.FullName -Destination (Join-Path $evidenceDir 'view_matrix.log') -Force
    $patterns = @('ShadowMode','shadow-sample','ShadowMode3','GloveFaceFilter','GloveIsolation','suppressed detached','IndependentHandsAudit')
    $signals = foreach ($p in $patterns) {
        Select-String -LiteralPath $log.FullName -Pattern $p | Select-Object -Last 8
    }
    $signals | ForEach-Object { $_.Line } | Set-Content (Join-Path $evidenceDir 'log-signals.txt') -Encoding UTF8
}

$capture = Get-ChildItem -LiteralPath $logRoot -Filter 'capture_*.bmp' -File |
    Sort-Object LastWriteTime -Descending | Select-Object -First 2
foreach ($c in $capture) {
    $destBmp = Join-Path $evidenceDir $c.Name
    Copy-Item -LiteralPath $c.FullName -Destination $destBmp -Force
    Add-Type -AssemblyName System.Drawing
    $img = [System.Drawing.Image]::FromFile($destBmp)
    $png = [System.IO.Path]::ChangeExtension($destBmp, '.png')
    $img.Save($png, [System.Drawing.Imaging.ImageFormat]::Png)
    $img.Dispose()
}

Write-Host "Evidence: $evidenceDir"
