Add-Type -AssemblyName System.Speech
$s = New-Object System.Speech.Synthesis.SpeechSynthesizer
$s.GetInstalledVoices() | ForEach-Object {
    $info = $_.VoiceInfo
    Write-Output ("{0}`t{1}" -f $info.Name, $info.Culture)
}
