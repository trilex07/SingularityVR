[CmdletBinding()]
param([ValidateRange(30,120)][int]$CaptureSeconds=90,
      [ValidateRange(30,180)][int]$LaunchWindowSeconds=120,
      [switch]$AfterCinemaExit,
      [ValidateRange(30,300)][int]$CinemaExitWindowSeconds=180)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'cinema-exit-trace-cue.ps1')
$identity=[Security.Principal.WindowsIdentity]::GetCurrent()
if(-not ([Security.Principal.WindowsPrincipal]::new($identity)).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw 'Windows CPU stack tracing requires an administrator token (UAC).'
}
$wpr=(Get-Command wpr.exe -ErrorAction Stop).Source
$status=(& $wpr -status | Out-String)
if($LASTEXITCODE -ne 0 -or $status -notmatch 'WPR is not recording') {
    throw 'Existing or unclear WPR session; refusing to interfere.'
}
$sessions=(& logman.exe query -ets | Out-String)
if($LASTEXITCODE -ne 0 -or $sessions -match 'WPR_initiated_') {
    throw 'Existing named WPR session or unclear ETW inventory; refusing to interfere.'
}
$instance='SingularityVRTrigger-'+[guid]::NewGuid().ToString('N')
$run=Join-Path (Split-Path $PSScriptRoot -Parent) ('deployment\shadow-probe\cpu-'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $run -ErrorAction Stop | Out-Null
Start-Transcript -LiteralPath (Join-Path $run 'recorder.log') | Out-Null
$trace=Join-Path $run 'trigger-cpu.etl'
$started=$false
try {
    if(-not $AfterCinemaExit) {
        & $wpr -start CPU.Verbose -filemode -instancename $instance
        if($LASTEXITCODE -ne 0) {throw 'WPR start failed; inspect the named instance before retrying.'}
        $started=$true
        Write-Output "READY CPU STACK TRACE: $run instance=$instance; no game/input/file changes"
    } else {
        Write-Output "READY CINEMA-EXIT CPU RECORDER: $run; waiting for actual owned profile marker, not recording yet"
    }
    $launchDeadline=(Get-Date).AddSeconds($LaunchWindowSeconds)
    $gameProcess=$null
    do {
        $running=@(Get-Process Singularity -ErrorAction SilentlyContinue)
        # Startup can briefly expose two entries. Never choose an arbitrary
        # first PID or combine two sessions; wait for an unambiguous process.
        if($running.Count -eq 1){$gameProcess=$running[0]}
        if(-not $gameProcess){Start-Sleep -Seconds 1}
    } until($gameProcess -or (Get-Date) -ge $launchDeadline)
    if(-not $gameProcess){throw 'No game launch within bounded window.'}
    $gameId=$gameProcess.Id
    $gameStart=$gameProcess.StartTime
    if($AfterCinemaExit) {
        $cueDeadline=(Get-Date).AddSeconds($CinemaExitWindowSeconds)
        $nativeLog=Join-Path $env:LOCALAPPDATA 'SingularityVR\view_matrix.log'
        $cueSeen=$false
        do {
            $sameGame=Get-Process -Id $gameId -ErrorAction SilentlyContinue
            if(-not $sameGame -or $sameGame.StartTime -ne $gameStart) {
                throw 'Observed game exited/restarted before cinema exit; no trace started.'
            }
            if(Test-Path -LiteralPath $nativeLog) {
                $state=Get-Item -LiteralPath $nativeLog
                if($state.LastWriteTime -ge $gameStart -and $state.Length -le 33554432) {
                    # ReadAllText uses FileShare.Read: this fails while the
                    # native logger is writing. Get-Content shares read/write.
                    # A transient rotation/deletion is only a missed poll.
                    try {
                        $text=Get-Content -LiteralPath $nativeLog -Raw -ErrorAction Stop
                        if($text){$cueSeen=Test-CinemaExitTraceCue -Text $text -GameStart $gameStart}
                    } catch [IO.IOException] { $cueSeen=$false }
                }
            }
            if(-not $cueSeen){Start-Sleep -Seconds 2}
        } until($cueSeen -or (Get-Date) -ge $cueDeadline)
        if(-not $cueSeen){throw 'No fresh cinema-exit profile marker within bounded window; no trace started.'}
        # Another profiler may have started while we waited. Check again;
        # never interfere with somebody else's recording.
        $status=(& $wpr -status | Out-String)
        if($LASTEXITCODE -ne 0 -or $status -notmatch 'WPR is not recording') {throw 'WPR session appeared while waiting.'}
        $sessions=(& logman.exe query -ets | Out-String)
        if($LASTEXITCODE -ne 0 -or $sessions -match 'WPR_initiated_') {throw 'Named WPR session appeared while waiting.'}
        & $wpr -start CPU.Verbose -filemode -instancename $instance
        if($LASTEXITCODE -ne 0){throw 'WPR start failed; inspect the named instance before retrying.'}
        $started=$true
        Write-Output "CPU TRACE STARTED AFTER CINEMA EXIT: PID $gameId instance=$instance"
    }
    Write-Output "GAME PID $gameId; capture bounded to $CaptureSeconds seconds or process exit"
    $deadline=(Get-Date).AddSeconds($CaptureSeconds)
    while((Get-Date) -lt $deadline) {
        $sameGame=Get-Process -Id $gameId -ErrorAction SilentlyContinue
        if(-not $sameGame -or $sameGame.StartTime -ne $gameStart){break}
        Start-Sleep -Seconds 1
    }
} catch {
    # Hidden RunAs shells otherwise lose the terminating exception after the
    # transcript closes. Preserve the diagnostic before cleanup/rethrow.
    Write-Output ("CPU RECORDER ERROR: " + $_.Exception.Message)
    throw
} finally {
    if($started) {
        & $wpr -stop $trace -skipPdbGen -instancename $instance
        if($LASTEXITCODE -ne 0){throw "Owned WPR stop failed; instance=$instance. Never cancel an unrelated session."}
        Write-Output "SAVED CPU STACK TRACE: $trace (private system-wide ETL; analyze Singularity only)"
    }
    $log='C:\Users\Trilex\AppData\Local\SingularityVR\view_matrix.log'
    if(Test-Path -LiteralPath $log){Copy-Item -LiteralPath $log -Destination (Join-Path $run 'view_matrix.log')}
    Stop-Transcript | Out-Null
}
