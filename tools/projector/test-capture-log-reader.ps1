$ErrorActionPreference='Stop'
. "$PSScriptRoot/capture-log-reader.ps1"
function Assert($ok,$message) { if (-not $ok) { throw $message } }
$path=[IO.Path]::GetTempFileName()
try {
    $utf8=[Text.UTF8Encoding]::new($false)
    $line=('x'*120)+'ąęł'
    [IO.File]::WriteAllText($path, (($line+"`r`n")*12000)+'partial',$utf8)
    $state=New-CaptureLogTail
    $count=0
    while ($state.Offset -lt (Get-Item $path).Length) {
        foreach ($entry in @(Read-CaptureLogLines $path $state -ByteBudget 65537)) {
            Assert ($entry -ceq $line) 'Lost/corrupted UTF8 line in large backlog'
            $count++
        }
    }
    Assert ($count -eq 12000 -and $state.Fragment -eq 'partial') 'Backlog/fragment mismatch'
    [IO.File]::AppendAllText($path," completed`n",$utf8)
    $result=@(Read-CaptureLogLines $path $state)
    Assert ($result.Count -eq 1 -and $result[0] -eq 'partial completed') 'Append mismatch'
    [IO.File]::WriteAllText($path,"reset`n",$utf8)
    $result=@(Read-CaptureLogLines $path $state)
    Assert ($result.Count -eq 1 -and $result[0] -eq 'reset') 'Truncation mismatch'
    $cue=@{Paused=$false;Beats=0}
    $pause='menu-submit-edge: class=3 test'
    $resume='menu-submit-edge: class=0 test'
    $beat='xr-heartbeat: session=5 running=1 shouldRender=1 haveEyes=1 test'
    [void](Update-ProjectorCaptureCue $cue $pause)
    1..5 | ForEach-Object { [void](Update-ProjectorCaptureCue $cue $beat) }
    Assert (-not (Update-ProjectorCaptureCue $cue $resume)) 'Short pause triggered'
    [void](Update-ProjectorCaptureCue $cue $pause)
    1..6 | ForEach-Object { [void](Update-ProjectorCaptureCue $cue $beat) }
    Assert (Update-ProjectorCaptureCue $cue $resume) 'Long pause did not trigger'
    Assert (-not (Update-ProjectorCaptureCue $cue $resume)) 'Repeated trigger'
    [void](Update-ProjectorCaptureCue $cue $pause)
    1..6 | ForEach-Object { [void](Update-ProjectorCaptureCue $cue $beat) }
    [void](Update-ProjectorCaptureCue $cue 'menu-submit-edge: class=4 test')
    Assert (-not (Update-ProjectorCaptureCue $cue $resume)) 'Options exit triggered'
    'PASS: >1 MiB backlog, UTF8 boundaries, append, truncation, short/long pause and options cues'
} finally { [IO.File]::Delete($path) }
