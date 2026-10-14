$ErrorActionPreference='Stop'
$tokens=$null;$errors=$null
$ast=[Management.Automation.Language.Parser]::ParseFile((Join-Path $PSScriptRoot '../polish/install-story-xlingual.ps1'),[ref]$tokens,[ref]$errors)
if($errors.Count){throw 'Syntax errors'}
$waveFunction=$ast.Find({param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -ceq 'Wave'},$true)
if(-not $waveFunction){throw 'Missing format validation'}
. ([scriptblock]::Create($waveFunction.Extent.Text)) # definition only, no installer execution
$folder=Join-Path ([IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../deployment'))) ('story-format-test-'+[Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $folder | Out-Null
$bytes=[byte[]]::new(48)
foreach($pair in @(@(0,'RIFF'),@(8,'WAVE'),@(12,'fmt '),@(36,'data'))){[Text.Encoding]::ASCII.GetBytes($pair[1]).CopyTo($bytes,$pair[0])}
[BitConverter]::GetBytes([uint32]40).CopyTo($bytes,4)
[BitConverter]::GetBytes([uint32]16).CopyTo($bytes,16)
foreach($pair in @(@(20,1),@(22,1),@(32,2),@(34,16))){[BitConverter]::GetBytes([uint16]$pair[1]).CopyTo($bytes,$pair[0])}
[BitConverter]::GetBytes([uint32]24000).CopyTo($bytes,24)
[BitConverter]::GetBytes([uint32]48000).CopyTo($bytes,28)
[BitConverter]::GetBytes([uint32]4).CopyTo($bytes,40)
$path=Join-Path $folder 'fixture.wav';[IO.File]::WriteAllBytes($path,$bytes);Wave $path
foreach($offset in @(0,4,8,16,20,22,24,32,34,40)){
 $bad=$bytes.Clone();$bad[$offset]=$bad[$offset] -bxor 255
 [IO.File]::WriteAllBytes($path,$bad);$refused=$false
 try{Wave $path}catch{$refused=$true}
 if(-not $refused){throw "Malformed WAV accepted at $offset"}
}
[IO.File]::WriteAllBytes($path,$bytes)
Write-Output 'PASS: mono PCM16/24kHz fixture and ten invalid-container/format refusals; no Steam writes'
