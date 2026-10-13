[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$EvidenceDirectory,
    [ValidateRange(60,1800)][int]$ObserveSeconds = 900,
    [ValidateRange(60,600)][int]$LaunchWindowSeconds = 600
)
# MAIN-only read-only observer. No input, process termination, or install writes.
# The independently armed post-exit restore guard owns DLL cleanup.
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$output = (Resolve-Path -LiteralPath $EvidenceDirectory).Path
$log = Join-Path $env:LOCALAPPDATA 'SingularityVR\view_matrix.log'
$started = Get-Date
$launchDeadline = $started.AddSeconds($LaunchWindowSeconds)
$deadline = $started.AddSeconds($ObserveSeconds)
$seen = [Collections.Generic.HashSet[string]]::new()
$events = [Collections.Generic.List[string]]::new()
$gameId = 0
$heartbeat = $started
$pattern = 'OpenXR ready|session state|SceneMode:|camera cut|level transition:|cutscene-mono:|cutscene STARTED|cutscene ENDED|bCinematicMode|yaw asserted by the ENGINE|FRAME SPIKE|VR frame:|fps='
Write-Output ('OBSERVER READY: read-only; deadline ' + $deadline.ToString('o'))
while ((Get-Date) -lt $deadline) {
    $running = @(Get-Process -Name Singularity -ErrorAction SilentlyContinue)
    if ($running.Count -gt 1) { throw 'Multiple game processes; observation identity ambiguous' }
    if ($running.Count) {
        if (-not $gameId) {
            $gameId = $running[0].Id
            Write-Output "GAME OBSERVED: PID $gameId"
        } elseif ($running[0].Id -ne $gameId) {
            Write-Output 'Game restarted; stop rather than mix sessions'
            break
        }
    } elseif ($gameId) {
        Write-Output "GAME EXITED: PID $gameId"
        break
    } elseif ((Get-Date) -ge $launchDeadline) {
        Write-Output 'NO LAUNCH: bounded window expired'
        break
    }
    if ($gameId -and (Test-Path -LiteralPath $log)) {
        $state = Get-Item -LiteralPath $log
        if ($state.LastWriteTime -ge $started) {
            foreach ($line in @(Get-Content -LiteralPath $log -Tail 240 -ErrorAction SilentlyContinue)) {
                if ($line -match $pattern -and $seen.Count -lt 1000 -and $seen.Add($line)) {
                    $entry = (Get-Date).ToString('o') + ' observed: ' + $line
                    $events.Add($entry)
                    Write-Output $entry
                }
            }
        }
    }
    if ((Get-Date) -ge $heartbeat) {
        Write-Output ('OBSERVING: PID=' + $gameId + '; events=' + $events.Count)
        $heartbeat = (Get-Date).AddSeconds(30)
    }
    Start-Sleep -Seconds 5
}
if (Test-Path -LiteralPath $log) {
    $final = Get-Item -LiteralPath $log
    if ($gameId -and $final.LastWriteTime -ge $started -and $final.Length -le 33554432) {
        Copy-Item -LiteralPath $log -Destination (Join-Path $output 'session.log')
    } else { Write-Output 'Full log not copied: stale, no session, or exceeds 32 MiB bound' }
}
$events | Set-Content -LiteralPath (Join-Path $output 'observed-events.txt') -Encoding UTF8
[ordered]@{
    startedAt = $started.ToString('o'); finishedAt = (Get-Date).ToString('o')
    gameProcessId = $gameId; eventCount = $events.Count
    gameStillRunning = [bool](@(Get-Process -Name Singularity -ErrorAction SilentlyContinue).Count)
    note = 'Observation timestamps, not native event times. Live tail can miss bursts; session.log is authoritative. No visual acceptance inferred.'
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'observation.json') -Encoding UTF8
Write-Output 'OBSERVER FINISHED: evidence saved; restore guard is independent'
