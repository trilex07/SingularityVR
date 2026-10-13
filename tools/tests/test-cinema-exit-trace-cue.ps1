$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot '..\cinema-exit-trace-cue.ps1')
$started=[datetime]'2026-10-13T19:15:00.500'
$old="=== view_matrix attached 2026-10-13 18:54:29 - run starts here ===`nCINEMA_EXIT_HOTPATH armed: old"
$fresh="=== view_matrix attached 2026-10-13 19:15:00 - run starts here ===`n"
if(Test-CinemaExitTraceCue $old $started){throw 'Stale session accepted'}
if(Test-CinemaExitTraceCue ($old+"`n"+$fresh) $started){throw 'Appended stale cue accepted'}
if(Test-CinemaExitTraceCue $fresh $started){throw 'Header alone accepted'}
if(Test-CinemaExitTraceCue 'CINEMA_EXIT_HOTPATH armed: no session' $started){throw 'Headerless cue accepted'}
if(Test-CinemaExitTraceCue '=== view_matrix attached invalid - run starts here === CINEMA_EXIT_HOTPATH armed:' $started){throw 'Malformed header accepted'}
if(-not (Test-CinemaExitTraceCue ($fresh+'CINEMA_EXIT_HOTPATH armed: current') $started)){throw 'Fresh cue rejected'}
if(-not (Test-CinemaExitTraceCue ($old+"`n"+$fresh+'CINEMA_EXIT_HOTPATH armed: current') $started)){throw 'Fresh appended cue rejected'}
Write-Output 'PASS cinema-exit trace cue: fresh, appended old/current, missing, malformed, subsecond start'
