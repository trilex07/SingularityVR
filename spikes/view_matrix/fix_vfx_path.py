with open('build.ps1', 'r', encoding='utf-8') as f:
    content = f.read()

old = '''        # Copy SingularityVfx.dll (VFX plugin) if it exists
        $vfxDll = Join-Path $root "deployment\\desktop-hmd-current-20261009\\SingularityVfx.dll"
        if (Test-Path $vfxDll) {
            Copy-Item $vfxDll $devGameBin -Force
            Write-Host "Installed SingularityVfx.dll (VFX plugin) to: $devGameBin"
        } else {
            Write-Host "WARNING: SingularityVfx.dll not found at $vfxDll - VFX plugin will not be available" -ForegroundColor Yellow
        }'''

new = '''        # Copy SingularityVfx.dll (VFX plugin) if it exists
        # Try multiple stable locations
        $vfxDllPaths = @(
            Join-Path $root "deployment\\runtime-test-current\\SingularityVfx.dll",
            Join-Path $root "deployment\\desktop-hmd-current-20261009\\SingularityVfx.dll"
        )
        $vfxCopied = $false
        foreach ($vfxDll in $vfxDllPaths) {
            if (Test-Path $vfxDll) {
                Copy-Item $vfxDll $devGameBin -Force
                Write-Host "Installed SingularityVfx.dll (VFX plugin) to: $devGameBin"
                $vfxCopied = $true
                break
            }
        }
        if (-not $vfxCopied) {
            Write-Host "WARNING: SingularityVfx.dll not found in any known location - VFX plugin will not be available" -ForegroundColor Yellow
        }'''

content = content.replace(old, new)

with open('build.ps1', 'w', encoding='utf-8') as f:
    f.write(content)
print('VFX plugin path updated')