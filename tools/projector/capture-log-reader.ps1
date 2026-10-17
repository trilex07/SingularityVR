# Bounded incremental reader; backlog is consumed over successive calls.
# Keep UTF-8 decoder state and partial lines across byte boundaries.
function New-CaptureLogTail {
    param([long]$Offset=0)
    @{Offset=$Offset; Fragment=''; Decoder=[Text.Encoding]::UTF8.GetDecoder()}
}

function Read-CaptureLogLines {
    param([string]$Path, [hashtable]$State, [ValidateRange(16,1048576)][int]$ByteBudget=1048576)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return @() }
    $stream=[IO.File]::Open($Path,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite)
    try {
        if ($stream.Length -lt $State.Offset) {
            $State.Offset=0L; $State.Fragment=''; $State.Decoder.Reset()
        }
        $available=$stream.Length-$State.Offset
        if ($available -le 0) { return @() }
        $bytes=[byte[]]::new([int][Math]::Min($available,$ByteBudget))
        [void]$stream.Seek($State.Offset,[IO.SeekOrigin]::Begin)
        $count=$stream.Read($bytes,0,$bytes.Length)
        $State.Offset += $count
        $chars=[char[]]::new([Text.Encoding]::UTF8.GetMaxCharCount($count))
        $n=$State.Decoder.GetChars($bytes,0,$count,$chars,0,$false)
        $combined=$State.Fragment+[string]::new($chars,0,$n)
        $parts=$combined -split "`n",0,'SimpleMatch'
        $State.Fragment=$parts[-1]
        if ($State.Fragment.Length -gt 65536) { throw 'Capture log line exceeds 64 KiB budget' }
        if ($parts.Count -gt 1) { return @($parts[0..($parts.Count-2)] | ForEach-Object { $_.TrimEnd("`r") }) }
        return @()
    } finally { $stream.Dispose() }
}

function Update-ProjectorCaptureCue {
    param([hashtable]$State,[string]$Line)
    if ($Line -match '^menu-submit-edge: class=(\d+)\s') {
        $class=[int]$Matches[1]
        if ($class -eq 3) { $State.Paused=$true; $State.Beats=0 }
        else {
            $ready=$class -eq 0 -and $State.Paused -and $State.Beats -ge 6
            $State.Paused=$false; $State.Beats=0
            return $ready
        }
    } elseif ($State.Paused -and $Line -match '^xr-heartbeat: session=5 running=1 shouldRender=1 haveEyes=1\s') {
        $State.Beats=[Math]::Min($State.Beats+1,6)
    }
    return $false
}
