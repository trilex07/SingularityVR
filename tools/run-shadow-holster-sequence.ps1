[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidateRange(1,2147483647)][int]$GamePid,
    [ValidateRange(30,600)][int]$SyncTimeoutSeconds = 300,
    [ValidateRange(5,180)][int]$StageTimeoutSeconds = 120
)

# MAIN-owned external trace scheduler. The player stays in PSVR2 throughout.
# DRAWN synchronizes A/B; STORED synchronizes C. This never sends game input.
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$runtime = Join-Path $env:LOCALAPPDATA 'SingularityVR'
$logPath = Join-Path $runtime 'view_matrix.log'
$requestPath = Join-Path $runtime 'shadow-pass.request'
$copyRoot = Join-Path (Split-Path $PSScriptRoot -Parent) 'deployment\shadow-probe'
$script:logOffset = 0L
$script:fragment = ''
$script:ownedRequest = $null

function Confirm-Game {
    $process = Get-Process -Id $GamePid -ErrorAction SilentlyContinue
    if (-not $process -or $process.ProcessName -ne 'Singularity') {
        throw "The exact Singularity process $GamePid has ended"
    }
}

function Read-NewLogLines {
    if (-not (Test-Path -LiteralPath $logPath -PathType Leaf)) { return @() }
    $stream = [IO.File]::Open($logPath,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite)
    try {
        if ($stream.Length -lt $script:logOffset) {
            $script:logOffset = 0L
            $script:fragment = ''
        }
        $available = $stream.Length - $script:logOffset
        if ($available -eq 0) { return @() }
        if ($available -gt 1048576) { throw 'Log reader fell more than 1 MiB behind' }
        [void]$stream.Seek($script:logOffset,[IO.SeekOrigin]::Begin)
        $bytes = [byte[]]::new([int]$available)
        $count = $stream.Read($bytes,0,$bytes.Length)
        $script:logOffset += $count
        $combined = $script:fragment + [Text.Encoding]::UTF8.GetString($bytes,0,$count)
        $parts = $combined -split "`n",0,'SimpleMatch'
        if ($parts.Count -eq 1) {
            $script:fragment = $combined
            return @()
        }
        $script:fragment = $parts[-1]
        return @($parts[0..($parts.Count-2)] | ForEach-Object { $_.TrimEnd("`r") })
    } finally {
        $stream.Dispose()
    }
}

function Wait-Until([datetime]$target,[bool]$rejectEarlyStore) {
    while ((Get-Date) -lt $target) {
        Confirm-Game
        if ($rejectEarlyStore) {
            foreach ($line in @(Read-NewLogLines)) {
                if ($line -match 'weapon holster: STORED') {
                    throw 'Weapon was holstered before capture B'
                }
            }
        }
        Start-Sleep -Milliseconds 250
    }
}

function Request-Capture([int]$sequence,[string]$stage) {
    Confirm-Game
    if (Test-Path -LiteralPath $requestPath) { throw 'Another shadow trace request is pending' }
    $tracePath = Join-Path $runtime "shadow-pass-$GamePid-$sequence.txt"
    if (Test-Path -LiteralPath $tracePath) { throw "Trace sequence already exists: $tracePath" }
    & (Join-Path $PSScriptRoot 'request-shadow-pass-trace.ps1') -Label CONTROL | Out-Null
    # Present can consume the request immediately. A missing file is not a
    # scheduling failure; the completion log below is the actual success gate.
    $script:ownedRequest = $null
    try { $script:ownedRequest = [IO.File]::ReadAllText($requestPath) }
    catch [IO.FileNotFoundException] { }
    Write-Output "REQUESTED $stage sequence=$sequence"
    $deadline = (Get-Date).AddSeconds($StageTimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        Confirm-Game
        foreach ($line in @(Read-NewLogLines)) {
            if ($sequence -le 2 -and $line -match 'weapon holster: STORED') {
                throw "Weapon was holstered before capture $stage completed"
            }
            if ($line -match 'shadow-pass trace finished' -and
                $line.Contains("shadow-pass-$GamePid-$sequence.txt")) {
                if (-not (Test-Path -LiteralPath $tracePath)) { throw 'Completion log lacks trace file' }
                $saved = Join-Path $copyRoot "shadow-pass-$GamePid-$sequence.txt"
                Copy-Item -LiteralPath $tracePath -Destination $saved
                Write-Output "CAPTURED $stage $saved"
                $script:ownedRequest = $null
                return
            }
        }
        Start-Sleep -Milliseconds 250
    }
    throw "Timed out waiting for capture $stage"
}

try {
    Confirm-Game
    if (Test-Path -LiteralPath $requestPath) { throw 'Existing shadow trace request; refusing to start' }
    if (-not (Test-Path -LiteralPath $copyRoot -PathType Container)) {
        New-Item -ItemType Directory -Path $copyRoot | Out-Null
    }
    if (Test-Path -LiteralPath $logPath) {
        $script:logOffset = (Get-Item -LiteralPath $logPath).Length
    }
    Write-Output "WAITING_FOR_DRAWN pid=$GamePid"
    $deadline = (Get-Date).AddSeconds($SyncTimeoutSeconds)
    $drawnAt = $null
    while ((Get-Date) -lt $deadline -and -not $drawnAt) {
        Confirm-Game
        foreach ($line in @(Read-NewLogLines)) {
            if ($line -match 'weapon holster: DRAWN') {
                $drawnAt = Get-Date
                break
            }
        }
        if (-not $drawnAt) { Start-Sleep -Milliseconds 250 }
    }
    if (-not $drawnAt) { throw 'Timed out waiting for the VR holster DRAWN event' }
    Write-Output "SYNC_DRAWN $($drawnAt.ToString('o'))"
    Wait-Until $drawnAt.AddSeconds(4) $true
    Request-Capture 1 'A'
    Wait-Until $drawnAt.AddSeconds(16) $true
    Request-Capture 2 'B'
    Write-Output 'WAITING_FOR_STORED'
    $deadline = (Get-Date).AddSeconds($StageTimeoutSeconds)
    $storedAt = $null
    while ((Get-Date) -lt $deadline -and -not $storedAt) {
        Confirm-Game
        foreach ($line in @(Read-NewLogLines)) {
            if ($line -match 'weapon holster: STORED') {
                $storedAt = Get-Date
                break
            }
        }
        if (-not $storedAt) { Start-Sleep -Milliseconds 250 }
    }
    if (-not $storedAt) { throw 'Timed out waiting for the VR holster STORED event' }
    Write-Output "SYNC_STORED $($storedAt.ToString('o'))"
    Wait-Until $storedAt.AddSeconds(4) $false
    Request-Capture 3 'C'
    Write-Output 'COMPLETE: three bounded frames captured; user may close game'
} finally {
    if ($script:ownedRequest -and (Test-Path -LiteralPath $requestPath) -and
        [IO.File]::ReadAllText($requestPath) -eq $script:ownedRequest) {
        Remove-Item -LiteralPath $requestPath
        Write-Output 'Removed only this scheduler owned pending request'
    }
}
