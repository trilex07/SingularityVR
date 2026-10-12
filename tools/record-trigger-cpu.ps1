[CmdletBinding()]
param([ValidateRange(30,120)][int]$CaptureSeconds=90,
      [ValidateRange(30,180)][int]$LaunchWindowSeconds=120)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$identity=[Security.Principal.WindowsIdentity]::GetCurrent()
if(-not ([Security.Principal.WindowsPrincipal]::new($identity)).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw 'Windows CPU stack tracing requires an administrator token (UAC).'
}
$wpr=(Get-Command wpr.exe -ErrorAction Stop).Source
$status=(& $wpr -status | Out-String)
if($LASTEXITCODE -ne 0 -or $status -notmatch 'WPR is not recording') {
    throw 'Existing or unclear WPR session; refusing to interfere.'
}
$instance='SingularityVRTrigger-'+[guid]::NewGuid().ToString('N')
$run=Join-Path (Split-Path $PSScriptRoot -Parent) ('deployment\shadow-probe\cpu-'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $run -ErrorAction Stop | Out-Null
$trace=Join-Path $run 'trigger-cpu.etl'
$started=$false
try {
    & $wpr -start CPU.Verbose -filemode -instancename $instance
    if($LASTEXITCODE -ne 0) {throw 'WPR start failed; inspect the named instance before retrying.'}
    $started=$true
    Write-Output "READY CPU STACK TRACE: $run instance=$instance; no game/input/file changes"
    $launchDeadline=(Get-Date).AddSeconds($LaunchWindowSeconds)
    $gameProcess=$null
    do {
        $gameProcess=Get-Process Singularity -ErrorAction SilentlyContinue | Select-Object -First 1
        if(-not $gameProcess){Start-Sleep -Seconds 1}
    } until($gameProcess -or (Get-Date) -ge $launchDeadline)
    if(-not $gameProcess){throw 'No game launch within bounded window.'}
    $gameId=$gameProcess.Id
    Write-Output "GAME PID $gameId; capture bounded to $CaptureSeconds seconds or process exit"
    $deadline=(Get-Date).AddSeconds($CaptureSeconds)
    while((Get-Date) -lt $deadline -and (Get-Process -Id $gameId -ErrorAction SilentlyContinue)) {
        Start-Sleep -Seconds 1
    }
} finally {
    if($started) {
        & $wpr -stop $trace -skipPdbGen -instancename $instance
        if($LASTEXITCODE -ne 0){throw "Owned WPR stop failed; instance=$instance. Never cancel an unrelated session."}
        Write-Output "SAVED CPU STACK TRACE: $trace (private system-wide ETL; analyze Singularity only)"
    }
    $log='C:\Users\Trilex\AppData\Local\SingularityVR\view_matrix.log'
    if(Test-Path -LiteralPath $log){Copy-Item -LiteralPath $log -Destination (Join-Path $run 'view_matrix.log')}
}
