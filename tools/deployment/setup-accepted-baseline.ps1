param(
    [string]$SteamGameRoot = 'C:\Program Files (x86)\Steam\steamapps\common\Singularity'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
. (Join-Path $PSScriptRoot 'AcceptedProfileGuard.ps1')

$inventory = @(
    'Binaries/d3d9.dll',
    'Binaries/dinput8.dll',
    'Binaries/dinput8_orig.dll',
    'Binaries/texture_override.ini',
    'Binaries/vr-assets/hud/katorga/glove-animation/native_hands.svni',
    'Binaries/vr-assets/hud/katorga/glove-animation/hand_thumb.svnt',
    'Binaries/vr-assets/hud/katorga/glove-animation/native_cuffs.svnc',
    'Binaries/vr-assets/knife/knife.svrk',
    'Binaries/vr-assets/knife/knife.svrt',
    'Binaries/vr-assets/knife/knife_hand.svng',
    'Binaries/vr-assets/knife/knife.svnl',
    'Binaries/vr-assets/knife/knife_normal.svrt',
    'Binaries/vr-assets/knife/knife_specular.svrt',
    'Binaries/vr-assets/localization/pl/polish_text.jsonl',
    'Binaries/vr-assets/localization/pl/polish_voice.jsonl',
    'Binaries/vr-assets/localization/pl/polish_story_voice.jsonl'
)

$profile = New-AcceptedProfileDraft -GameRoot $SteamGameRoot -RelativeFiles $inventory -IniRelativePath 'Binaries/SingularityVR.ini'
$profile.approved = $true
$profile.acceptedBy = 'MAIN'
$profile.acceptanceScope = 'Camera stabilization, melee shake suppression, Polish localization, knife, hands, hud plates'

$profilePath = Join-Path $repoRoot 'deployment/accepted-profile.json'
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $profilePath) | Out-Null
$profileJson = $profile | ConvertTo-Json -Depth 10
Set-Content -LiteralPath $profilePath -Value $profileJson -Encoding UTF8
$profileHash = Get-GuardHash $profilePath
Write-Host "Created approved accepted-profile: $profilePath (SHA256: $profileHash)"

# Build candidate change plan for PR #40 + camera stabilization
$candidateDll = Join-Path $repoRoot 'spikes/view_matrix/d3d9.dll'
$candidateDllHash = Get-GuardHash $candidateDll
$currentSteamDll = Join-Path $SteamGameRoot 'Binaries/d3d9.dll'
$currentSteamDllHash = Get-GuardHash $currentSteamDll

$plan = [pscustomobject]@{
    schema = 'singularityvr-change-plan-v1'
    profileSha256 = $profileHash
    reason = 'PR #40 frontend menu comfort isolation and camera stabilization'
    files = @(
        [pscustomobject]@{
            path = 'Binaries/d3d9.dll'
            before = $currentSteamDllHash
            after = $candidateDllHash
            reason = 'PR #40 menu comfort and sprint/melee camera stabilization'
        }
    )
    ini = @(
        [pscustomobject]@{
            key = 'Render/FrontMenuCleanBackdrop'
            before = $null
            after = '1'
            reason = 'PR #40 clean frontend backdrop'
        },
        [pscustomobject]@{
            key = 'Render/FrontMenuScalePct'
            before = '88'
            after = '80'
            reason = 'PR #40 frontend comfort scale'
        },
        [pscustomobject]@{
            key = 'Render/FrontMenuDistanceMm'
            before = '1400'
            after = '1800'
            reason = 'PR #40 frontend comfort distance'
        }
    )
}

$planPath = Join-Path $repoRoot 'deployment/change-plan-menu-comfort.json'
$planJson = $plan | ConvertTo-Json -Depth 10
Set-Content -LiteralPath $planPath -Value $planJson -Encoding UTF8
$planHash = Get-GuardHash $planPath
Write-Host "Created change plan: $planPath (SHA256: $planHash)"
