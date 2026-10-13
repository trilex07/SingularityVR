[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$request='C:\Users\Trilex\AppData\Local\SingularityVR\hotpath-gameplay.request'
if(Test-Path -LiteralPath $request) { throw 'Gameplay hotpath request already exists' }
[IO.File]::WriteAllText($request,'SingularityVR gameplay hotpath v1',[Text.Encoding]::ASCII)
Write-Output 'One bounded existing-hook profile requested; waits for focused gameplay. Requires HotPathSampler=0. Does not modify INI or input.'
