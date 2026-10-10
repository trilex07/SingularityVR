[CmdletBinding()]
param(
    [string]$SteamRoot = 'C:\Program Files (x86)\Steam',
    [string]$CandidateDir = 'C:\Users\Trilex\vr-mods\deployment\desktop-hmd-diagnostic-20261008',
    [int]$StartupWaitSeconds = 60
)

$ErrorActionPreference = 'Stop'
$gameDir = Join-Path $SteamRoot 'steamapps\common\Singularity\Binaries'
$gameExe = Join-Path $gameDir 'Singularity.exe'
$steamExe = Join-Path $SteamRoot 'steam.exe'
$backupDir = Join-Path $CandidateDir ('probe-backup-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))

Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class DesktopHmdProbeNative {
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int cmd);
    [DllImport("user32.dll")] public static extern bool BringWindowToTop(IntPtr hWnd);
    [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll")] static extern bool EnumWindows(EnumProc cb, IntPtr lParam);
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint pid);
    delegate bool EnumProc(IntPtr hWnd, IntPtr lParam);
    public static IntPtr FindForProcess(int wanted) {
        IntPtr found=IntPtr.Zero;
        EnumWindows((h,p)=>{ uint id; GetWindowThreadProcessId(h,out id); if(id==(uint)wanted && IsWindowVisible(h)) { found=h; return false; } return true; }, IntPtr.Zero);
        return found;
    }
    [DllImport("user32.dll", SetLastError=true)] static extern uint SendInput(uint n, INPUT[] inputs, int size);
    [StructLayout(LayoutKind.Sequential)] struct INPUT { public uint type; public InputUnion data; }
    [StructLayout(LayoutKind.Explicit)] struct InputUnion {
        [FieldOffset(0)] public KEYBDINPUT ki;
        [FieldOffset(0)] public MOUSEINPUT mi;
    }
    [StructLayout(LayoutKind.Sequential)] struct KEYBDINPUT { public ushort wVk; public ushort wScan; public uint dwFlags; public uint time; public UIntPtr dwExtraInfo; }
    [StructLayout(LayoutKind.Sequential)] struct MOUSEINPUT { public int dx; public int dy; public uint mouseData; public uint dwFlags; public uint time; public UIntPtr dwExtraInfo; }
    public const uint INPUT_MOUSE=0, INPUT_KEYBOARD=1, KEYUP=0x0002, MOUSE_LEFTDOWN=0x0002, MOUSE_LEFTUP=0x0004;
    public static void Key(byte key, bool up) {
        INPUT[] a=new INPUT[1]; a[0].type=INPUT_KEYBOARD; a[0].data.ki.wVk=key; a[0].data.ki.dwFlags=up?KEYUP:0; SendInput(1,a,Marshal.SizeOf(typeof(INPUT)));
    }
    public static void Mouse(bool up) {
        INPUT[] a=new INPUT[1]; a[0].type=INPUT_MOUSE; a[0].data.mi.dwFlags=up?MOUSE_LEFTUP:MOUSE_LEFTDOWN; SendInput(1,a,Marshal.SizeOf(typeof(INPUT)));
    }
}
'@

function Focus-Game {
    $p = Get-Process -Name Singularity -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $p) { throw 'Singularity process not found' }
    $h = [DesktopHmdProbeNative]::FindForProcess($p.Id)
    if ($h -eq [IntPtr]::Zero) { $h = $p.MainWindowHandle }
    if ($h -eq [IntPtr]::Zero) { throw 'Singularity window not found' }
    [DesktopHmdProbeNative]::ShowWindow($h, 9) | Out-Null
    [DesktopHmdProbeNative]::BringWindowToTop($h) | Out-Null
    [DesktopHmdProbeNative]::SetForegroundWindow($h) | Out-Null
    Write-Host ("Focused Singularity window 0x{0:X}" -f $h.ToInt64())
    return $p
}
function Tap-Key([byte]$Vk, [int]$HoldMs = 100) {
    [DesktopHmdProbeNative]::Key($Vk, $false)
    Start-Sleep -Milliseconds $HoldMs
    [DesktopHmdProbeNative]::Key($Vk, $true)
}
function Hold-Key([byte]$Vk, [int]$HoldMs) {
    [DesktopHmdProbeNative]::Key($Vk, $false)
    Start-Sleep -Milliseconds $HoldMs
    [DesktopHmdProbeNative]::Key($Vk, $true)
}
function Synthetic-Fire([int]$HoldMs = 350) {
    [DesktopHmdProbeNative]::Mouse($false)
    Start-Sleep -Milliseconds $HoldMs
    [DesktopHmdProbeNative]::Mouse($true)
}

if (-not (Test-Path $CandidateDir)) { throw "Candidate not found: $CandidateDir" }
if (-not (Test-Path $gameExe)) { throw "Game not found: $gameExe" }
New-Item -ItemType Directory -Force -Path $backupDir | Out-Null
$files = @('d3d9.dll','SingularityVR.ini','SingularityVfx.dll')
try {
    Get-Process -Name Singularity -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 2
    foreach ($f in $files) {
        $dst = Join-Path $gameDir $f
        if (Test-Path $dst) { Copy-Item $dst (Join-Path $backupDir $f) -Force }
        if (Test-Path (Join-Path $CandidateDir $f)) { Copy-Item (Join-Path $CandidateDir $f) $dst -Force }
    }
    if (-not (Get-Process -Name steam -ErrorAction SilentlyContinue)) {
        Start-Process $steamExe -ArgumentList '-silent' | Out-Null
        Start-Sleep -Seconds 18
    }
    Start-Process $steamExe -WorkingDirectory $SteamRoot -ArgumentList '-applaunch','42670' | Out-Null
    $deadline = (Get-Date).AddSeconds($StartupWaitSeconds)
    do {
        $proc = Get-Process -Name Singularity -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($proc -and $proc.MainWindowHandle -ne [IntPtr]::Zero) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if (-not $proc) { throw 'Singularity did not start' }

    # Let the splash/menu settle, then use the native keyboard route for Continue.
    Start-Sleep -Seconds 6
    Focus-Game | Out-Null; Tap-Key 0x0D
    Start-Sleep -Seconds 2
    Focus-Game | Out-Null; Tap-Key 0x0D
    Start-Sleep -Seconds 5

    # NUMPAD3 enables the mod's deterministic XInput pad; NUMPAD4 holds the move stick forward.
    Focus-Game | Out-Null; Tap-Key 0x63 500
    Focus-Game | Out-Null; Tap-Key 0x64 500
    Start-Sleep -Seconds 5
    Focus-Game | Out-Null; Tap-Key 0x64 500

    # The engine's normal mouse-left fire path is used only as a synthetic test event.
    Focus-Game | Out-Null; Synthetic-Fire
    Start-Sleep -Seconds 4
    Focus-Game | Out-Null; Synthetic-Fire
    Start-Sleep -Seconds 12
} finally {
    Get-Process -Name Singularity -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 2
    foreach ($f in $files) {
        $src = Join-Path $backupDir $f
        if (Test-Path $src) { Copy-Item $src (Join-Path $gameDir $f) -Force }
    }
    Write-Host "Restored Steam files from $backupDir"
}
