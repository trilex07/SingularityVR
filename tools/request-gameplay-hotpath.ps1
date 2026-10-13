[CmdletBinding()]
param([switch]$AfterCinemaExit)
$ErrorActionPreference='Stop'
$request='C:\Users\Trilex\AppData\Local\SingularityVR\hotpath-gameplay.request'
if(Test-Path -LiteralPath $request) { throw 'Gameplay hotpath request already exists' }
$payload=if($AfterCinemaExit){'SingularityVR cinema exit hotpath v1'}else{'SingularityVR gameplay hotpath v1'}
[IO.File]::WriteAllText($request,$payload,[Text.Encoding]::ASCII)
Write-Output 'One bounded existing-hook profile requested; cinema-exit mode waits for actual exit and stops after45s. Requires HotPathSampler=0. Does not modify INI or input.'
