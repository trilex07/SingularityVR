param([Parameter(Mandatory)][int]$GameProcessId,
      [Parameter(Mandatory)][uint32]$TargetObject,
      [Parameter(Mandatory)][uint32]$MovieObject)
$ErrorActionPreference = 'Stop'
$process = Get-Process -Id $GameProcessId
if ($process.ProcessName -ne 'Singularity') { throw 'PID does not name Singularity' }
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class PauseRttMemory {
    [DllImport("kernel32.dll", SetLastError=true)] public static extern IntPtr OpenProcess(uint rights, bool inherit, int pid);
    [DllImport("kernel32.dll", SetLastError=true)] static extern bool ReadProcessMemory(IntPtr process, IntPtr address, byte[] bytes, UIntPtr count, out UIntPtr read);
    [DllImport("kernel32.dll")] public static extern bool CloseHandle(IntPtr handle);
    public static uint Word(IntPtr process, uint address) {
        byte[] bytes = new byte[4]; UIntPtr read;
        if (!ReadProcessMemory(process, new IntPtr((long)address), bytes, new UIntPtr(4), out read) || read.ToUInt64()!=4)
            throw new Exception("Read failed at " + address.ToString("X8"));
        return BitConverter.ToUInt32(bytes,0);
    }
}
'@
$handle = [PauseRttMemory]::OpenProcess(0x1010, $false, $GameProcessId)
if ($handle -eq [IntPtr]::Zero) { throw 'Read-only process access unavailable' }
function Word([uint32]$address) { [PauseRttMemory]::Word($handle, $address) }
function Hex([uint32]$value) { '{0:X8}' -f $value }
try {
    $resource = Word ($TargetObject + 0xA4)
    $rhi = if ($resource) { Word ($resource + 0x48) } else { 0 }
    $engine = Word 0x01CF8778
    $records = @()
    foreach ($list in @(@{kind='screen';offset=0x28},@{kind='RTT';offset=0x40})) {
        $data = Word ($engine + $list.offset)
        $count = Word ($engine + $list.offset + 4)
        if ($count -gt 64) { throw 'Unexpected movie list size' }
        for ($i=0; $i -lt $count; $i++) {
            $record = Word ($data + $i*4)
            if ($record -and (Word ($record + 0x50)) -eq $MovieObject) {
                $records += [ordered]@{kind=$list.kind; record=Hex $record; target=Hex (Word ($record+0x54));active=Word ($record+0x3C); timing=Word ($record+0x4C)}
            }
        }
    }
    $textureRhi = Word ($resource+0x64)
    $rhiWords = @(); $textureWords = @()
    for ($i=0; $i -lt 24; $i++) {
        if ($rhi) {$rhiWords += [ordered]@{offset=Hex ($i*4);value=Hex (Word ($rhi+$i*4))}}
        if ($textureRhi) {$textureWords += [ordered]@{offset=Hex ($i*4);value=Hex (Word ($textureRhi+$i*4))}}
    }
    $driver = Word 0x01CDA34C; $driverTable = Word $driver
    [ordered]@{pid=$GameProcessId;target=Hex $TargetObject;movie=Hex $MovieObject;
        resource=Hex $resource;owner=Hex (Word ($resource+0x60));rhi=Hex $rhi;
        nativeTexture=if($textureRhi){Hex (Word ($textureRhi-4))}else{'NULL'};
        nativeSurface=if($rhi){Hex (Word ($rhi-4))}else{'NULL'};
        nativeTextureClass=if($textureRhi){Hex (Word ($textureRhi-12))}else{'NULL'};
        nativeSurfaceClass=if($rhi){Hex (Word ($rhi-12))}else{'NULL'};
        textureRhi=Hex $textureRhi;records=$records;rhiWords=$rhiWords;textureWords=$textureWords;
        createTexture=Hex (Word ($driverTable+0x48));createSurface=Hex (Word ($driverTable+0x88))} | ConvertTo-Json -Depth 4
} finally { [PauseRttMemory]::CloseHandle($handle) | Out-Null }
