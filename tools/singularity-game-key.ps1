# Press keyboard keys into the Singularity window (scancode injection).
# Dishonored-VR's game-key.ps1 hardcodes process name "Dishonored"; this is the
# same logic for Steam Singularity (app 42670).
[CmdletBinding()]
param(
    [string]$Key = "",
    [int]$Scan = 0,
    [int]$Repeat = 1,
    [int]$Delay = 500,
    [int]$HoldMs = 60,
    [switch]$NoFocus
)

$ErrorActionPreference = 'Stop'
$scans = @{
    'space' = 0x39; 'enter' = 0x1C; 'return' = 0x1C; 'escape' = 0x01; 'esc' = 0x01
    'tab' = 0x0F; 'e' = 0x12; 'f' = 0x21; 'y' = 0x15; 'n' = 0x31
}
if ($Scan -eq 0) {
    if (-not $Key) { throw "give -Key <name> or -Scan <code>" }
    $k = $Key.ToLower()
    if (-not $scans.ContainsKey($k)) { throw "unknown key '$Key'" }
    $Scan = $scans[$k]
}

Add-Type -ErrorAction SilentlyContinue @'
using System; using System.Runtime.InteropServices;
public static class SgvKey {
  [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int n);
}
'@

$proc = 'Singularity'
$p = @(Get-Process $proc -ErrorAction SilentlyContinue | Where-Object { -not $_.HasExited })
if ($p.Count -gt 1) {
    $withWindow = @($p | Where-Object { $_.MainWindowHandle -ne [IntPtr]::Zero })
    if ($withWindow.Count -ge 1) { $p = $withWindow }
    $p = @($p | Sort-Object Id -Descending)
}
$p = $p | Select-Object -First 1
if (-not $p) { throw "$proc is not running" }

if (-not $NoFocus -and $p.MainWindowHandle -ne [IntPtr]::Zero) {
    [void][SgvKey]::ShowWindow($p.MainWindowHandle, 9)
    [void][SgvKey]::SetForegroundWindow($p.MainWindowHandle)
    Start-Sleep -Milliseconds 400
}

$KEYEVENTF_SCANCODE = 0x0008
$KEYEVENTF_KEYUP = 0x0002
for ($i = 0; $i -lt $Repeat; $i++) {
    [SgvKey]::keybd_event(0, [byte]$Scan, $KEYEVENTF_SCANCODE, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds $HoldMs
    [SgvKey]::keybd_event(0, [byte]$Scan, $KEYEVENTF_SCANCODE -bor $KEYEVENTF_KEYUP, [UIntPtr]::Zero)
    if ($i -lt $Repeat - 1) { Start-Sleep -Milliseconds $Delay }
}
Write-Output ("pressed scancode 0x{0:X2} x{1} in {2} (pid {3})" -f $Scan, $Repeat, $proc, $p.Id)
