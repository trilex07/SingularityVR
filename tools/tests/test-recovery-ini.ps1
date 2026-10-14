$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot '../recovery-ini.ps1')
foreach($eol in @("`n","`r`n")){
 $original=@('[Render]','MenuOverlay=0','GunAnchorFwd=30','[Input]','RecenterChord=1',
             '[Localization]','UiNarratorEnabled=0','[Diagnostic]','IndependentHandsAudit=1') -join $eol
 $replaced=Set-RecoveryIniKey $original 'Render' 'MenuOverlay' '1'
 if($replaced -cne $original.Replace('MenuOverlay=0','MenuOverlay=1')){throw 'Unowned bytes changed'}
 $inserted=Set-RecoveryIniKey $original 'Localization' 'NativePolishSpeech' '1'
 if($inserted -cne $original.Replace('[Localization]','[Localization]'+$eol+'NativePolishSpeech=1')){throw 'Insertion crossed a section'}
 $disabled=Set-RecoveryIniKey $original 'Diagnostic' 'IndependentHandsAudit' '0'
 if($disabled -cne $original.Replace('IndependentHandsAudit=1','IndependentHandsAudit=0')){throw 'Last section update failed'}
 $duplicate=$original.Replace('MenuOverlay=0','MenuOverlay=0'+$eol+'MenuOverlay=1')
 $failed=$false;try{$null=Set-RecoveryIniKey $duplicate 'Render' 'MenuOverlay' '1'}catch{$failed=$true}
 if(-not $failed){throw 'Duplicate key accepted'}
 $failed=$false;try{$null=Set-RecoveryIniKey ($original+$eol+'[Render]') 'Render' 'MenuOverlay' '1'}catch{$failed=$true}
 if(-not $failed){throw 'Duplicate section accepted'}
}
Write-Output 'PASS: LF/CRLF exact scoped edits, narrator/recenter/calibration preserved, duplicates refused'
