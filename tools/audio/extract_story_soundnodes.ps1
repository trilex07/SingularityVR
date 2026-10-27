$ErrorActionPreference = 'Stop'
$gameCooked = 'C:\Program Files (x86)\Steam\steamapps\common\Singularity\RvGame\CookedPC'
$umodel = 'C:\Users\Trilex\vr-mods\research\tools\UEViewer\umodel.exe'
$outRoot = 'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\story_en_reference'
$log = Join-Path $outRoot 'extract.log'
New-Item -ItemType Directory -Force -Path $outRoot | Out-Null
$packages = Get-ChildItem $gameCooked -Filter 'SP_*_LOC_INT.xxx' | Sort-Object Name
$i = 0
foreach ($pkg in $packages) {
    $i++
    $leaf = [IO.Path]::GetFileNameWithoutExtension($pkg.Name)
    $target = Join-Path $outRoot $leaf
    $existing = Get-ChildItem $target -Recurse -ErrorAction SilentlyContinue | Where-Object { -not $_.PSIsContainer -and $_.Extension -eq '.ogg' } | Measure-Object
    if ($existing.Count -gt 0) {
        Add-Content $log "[$i/$($packages.Count)] skip $($pkg.Name) ($($existing.Count) ogg)"
        continue
    }
    Add-Content $log "[$i/$($packages.Count)] export $($pkg.Name)"
    & $umodel -game=sing -path=$gameCooked -export -sounds -out=$outRoot $pkg.FullName *>> $log
    if ($LASTEXITCODE -ne 0) { Add-Content $log "ERROR exit=$LASTEXITCODE package=$($pkg.Name)" }
}
Add-Content $log 'complete'
