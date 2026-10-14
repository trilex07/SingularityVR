Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$script = Join-Path $repo 'tools/polish/deploy-polish-profile.ps1'
$tokens = $null
$errors = $null
[void][Management.Automation.Language.Parser]::ParseFile($script, [ref]$tokens, [ref]$errors)
if ($errors.Count) { throw ($errors | ForEach-Object { $_.Message }) -join '; ' }

$out = @(& $script -Action Plan -Profile first-area-l4-reference,verify-promoted -RepoRoot $repo 2>&1)
if (-not $?) { throw "Plan failed: $($out -join '; ')" }
$text = $out -join "`n"
if ($text -notmatch 'FORBIDDEN.*install_polish') { throw 'Plan must forbid install_polish.ps1' }
if ($text -notmatch 'first-area-l4-reference') { throw 'Plan missing reference profile' }

$out2 = @(& $script -Action Plan -Profile story-xlingual-v1 -RepoRoot $repo 2>&1)
if (($out2 -join "`n") -notmatch '992') { throw 'Story profile plan should mention 992 WAV scope' }

Write-Output 'PASS: deploy-polish-profile.ps1 parses and Plan runs offline'
