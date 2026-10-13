# Pure log qualification; never accept a cue left in an appended prior session.
function Test-CinemaExitTraceCue {
    param([string]$Text, [datetime]$GameStart)
    $header='=== view_matrix attached '
    $offset=$Text.LastIndexOf($header,[StringComparison]::Ordinal)
    if($offset -lt 0){return $false}
    $session=$Text.Substring($offset)
    if($session -cnotmatch '^=== view_matrix attached (\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}) - run starts here ==='){return $false}
    $stamp=[datetime]::MinValue
    if(-not [datetime]::TryParseExact($Matches[1],'yyyy-MM-dd HH:mm:ss',
        [Globalization.CultureInfo]::InvariantCulture,[Globalization.DateTimeStyles]::None,[ref]$stamp)){return $false}
    $startSecond=$GameStart.AddTicks(-($GameStart.Ticks % [TimeSpan]::TicksPerSecond))
    if($stamp -lt $startSecond){return $false}
    return $session.Contains('CINEMA_EXIT_HOTPATH armed:')
}
