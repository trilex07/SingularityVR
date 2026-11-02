$ErrorActionPreference='Stop'
$repo=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$out=Join-Path $repo ('build/pause-lease-profile/'+[guid]::NewGuid().ToString('N'))
[void][IO.Directory]::CreateDirectory($out)
'{}' | Set-Content (Join-Path $out 'manifest.json')
$log=Join-Path $out 'receipt.txt'
# Exercise the actual watcher, not source text. A missing profile must fail
# before READY, any game wait, hash of an installed target or rollback call.
$watcher=Join-Path $repo 'tools/watch-pause-test-restore.ps1'
$proc=Start-Process powershell -ArgumentList @('-NoProfile','-ExecutionPolicy','Bypass','-File',('"'+$watcher+'"'),'-BackupPath',('"'+$out+'"'),'-CandidateDllSha256',('0'*64),'-DllOnly') -WindowStyle Hidden -RedirectStandardOutput $log -RedirectStandardError ($log+'.err') -Wait -PassThru
if($proc.ExitCode -eq 0){throw 'Missing profile accepted'}
$text=(Get-Content $log -Raw)+(Get-Content ($log+'.err') -Raw)
if($text -match 'READY:' -or $text -notmatch 'requires explicit AcceptedProfilePath and ChangePlanPath') {throw 'Wrong refusal boundary'}
Write-Output 'PASS actual lease rejects missing deployment profile before READY; no Steam access'
