[CmdletBinding(SupportsShouldProcess)]
param(
    [ValidateSet('Plan', 'Install', 'Verify', 'Rollback')]
    [string]$Action = 'Plan',
    [ValidateSet('story-xlingual-v1', 'assets-v4-first-area', 'legacy-revoice-v2', 'verify-promoted', 'first-area-l4-reference')]
    [string[]]$Profile = @('verify-promoted'),
    [string]$GameRoot = 'C:\Program Files (x86)\Steam\steamapps\common\Singularity',
    [string]$RepoRoot = '',
    [string]$EvidenceDirectory = '',
    [string]$StorySourceDirectory = 'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\polish\story_voicepack_xlingual',
    [string]$AssetManifestPath = '',
    [string]$RevoiceCandidateDirectory = '',
    [switch]$AllowLegacyRevoice,
    [switch]$AllowPendingQa
)
# MAIN-only for Install/Rollback on real Steam. SIDE uses Plan/Verify offline.
# Composes existing narrow installers; never calls tools/polish/install_polish.ps1.
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not $RepoRoot) {
    $RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
}
if (-not $AssetManifestPath) {
    $AssetManifestPath = Join-Path $RepoRoot 'deployment/hands-dubbing/recovery-ready/assets/asset-manifest.json'
}
if (-not $RevoiceCandidateDirectory) {
    $RevoiceCandidateDirectory = 'C:\Users\Trilex\vr-mods\SingularityVR-post-opening\deployment\hands-dubbing\post-opening\quiet-revoice-v2'
}

function Assert-GameClosed {
    if (-not (Get-CimInstance Win32_Process -Filter "ProcessId=$PID")) { throw 'Full process visibility required' }
    if (@(Get-CimInstance Win32_Process -Filter "Name='Singularity.exe'").Count) { throw 'Singularity.exe running; no deployment writes' }
}

function Resolve-Evidence([string]$Root) {
    $full = [IO.Path]::GetFullPath($Root)
    if (-not $full.StartsWith($RepoRoot + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw 'EvidenceDirectory must be under deployment/ in this worktree'
    }
    if ($Action -eq 'Install' -and (Test-Path -LiteralPath $full)) { throw 'EvidenceDirectory must not exist before Install' }
    if ($Action -eq 'Rollback' -and -not (Test-Path -LiteralPath $full)) { throw 'EvidenceDirectory must exist for Rollback' }
    return $full
}

function Get-ProfileSpec {
    return @{
        'story-xlingual-v1'        = @{
            Description = '992 story WAVs only (xlingual drop-in); does not touch native UE3 banks, UI894, intro, DLL/INI.'
            Writes      = @('Binaries\vr-assets\audio\polish\story\*.wav (992)')
            Tool        = 'install-story-xlingual.ps1'
        }
        'assets-v4-first-area'     = @{
            Description = '38-file first-area delta v4: VI2 banks, PL manifests, ui_GFx_fonts, DDS, texture_override.ini.'
            Writes      = @('38 paths in recovery-ready/assets/asset-manifest.json')
            Tool        = 'test-prepared-assets.ps1'
            Requires    = 'Game baselines must match manifest BaselineSha256 (stock or known rollback state).'
        }
        'legacy-revoice-v2'        = @{
            Description = '1886 UI+story WAV overlay (quiet-revoice-v2). Does NOT replace native gameplay audio.'
            Writes      = @('Binaries\vr-assets\audio\polish\ui (894)', 'Binaries\vr-assets\audio\polish\story (992)')
            Tool        = 'install-revoice-candidate.ps1'
            Danger      = 'Can overwrite story WAVs installed by story-xlingual-v1; use only with explicit -AllowLegacyRevoice.'
        }
        'verify-promoted'          = @{
            Description = 'Read-only SHA check: 3 runtime + 38 assets + 992 story vs pinned manifests/receipt.'
            Writes      = @()
            Tool        = 'verify-polish-install-state.ps1'
        }
        'first-area-l4-reference'  = @{
            Description = 'Documentation profile only (PID37660): core DLL lease + assets-v4 + optional legacy-revoice; not a single auto bundle.'
            Writes      = @('See docs/polish-deploy-profiles.md')
            Tool        = 'Plan'
        }
    }
}

function Write-Plan {
    $spec = Get-ProfileSpec
    Write-Output "ACTION: $Action"
    Write-Output "GameRoot: $GameRoot"
    Write-Output "RepoRoot: $RepoRoot"
    foreach ($name in $Profile) {
        if (-not $spec.ContainsKey($name)) { throw "Unknown profile: $name" }
        $p = $spec[$name]
        Write-Output ""
        Write-Output "PROFILE: $name"
        Write-Output ("  " + $p.Description)
        foreach ($w in $p.Writes) { Write-Output ("  writes: " + $w) }
        if ($p.Tool) { Write-Output ("  tool: tools/polish or tools/audio/" + $p.Tool) }
        if ($p.ContainsKey('Requires') -and $p.Requires) { Write-Output ("  requires: " + $p.Requires) }
        if ($p.ContainsKey('Danger') -and $p.Danger) { Write-Output ("  danger: " + $p.Danger) }
    }
    Write-Output ""
    Write-Output 'FORBIDDEN: tools/polish/install_polish.ps1, tools/run-recovery-trial.ps1 (stale quiet-revoice baselines).'
    Write-Output 'Native bank rebuild: tools/audio/prepare_native_story.py (offline); not invoked by this orchestrator.'
}

function Invoke-StoryStep([string]$Step, [string]$Evidence) {
    $storyEvidence = Join-Path $Evidence 'story-xlingual'
    $installer = Join-Path $PSScriptRoot 'install-story-xlingual.ps1'
    switch ($Step) {
        'Preflight' { & $installer -Action Preflight -GameRoot $GameRoot -EvidenceDirectory $storyEvidence -SourceDirectory $StorySourceDirectory }
        'Install' {
            if (-not $PSCmdlet.ShouldProcess($GameRoot, 'Install story-xlingual-v1 (992 WAVs)')) { return }
            & $installer -Action Install -GameRoot $GameRoot -EvidenceDirectory $storyEvidence -SourceDirectory $StorySourceDirectory
        }
        'Rollback' { & $installer -Action Rollback -GameRoot $GameRoot -EvidenceDirectory $storyEvidence }
    }
}

function Invoke-AssetsStep([string]$Step, [string]$Evidence, [ref]$BackupPathOut) {
    if (-not (Test-Path -LiteralPath $AssetManifestPath)) { throw "Missing asset manifest: $AssetManifestPath" }
    $tool = Join-Path $RepoRoot 'tools/test-prepared-assets.ps1'
    switch ($Step) {
        'Preflight' { & $tool -Action Preflight -ManifestPath $AssetManifestPath -GameRoot $GameRoot }
        'Install' {
            if (-not $PSCmdlet.ShouldProcess($GameRoot, 'Install assets-v4-first-area (38 files)')) { return }
            $lines = @(& $tool -Action Install -ManifestPath $AssetManifestPath -GameRoot $GameRoot 2>&1)
            $lines | ForEach-Object { Write-Output $_ }
            $backupLine = $lines | Where-Object { $_ -match '^ASSET BACKUP:\s*(.+)$' } | Select-Object -Last 1
            if (-not $backupLine) { throw 'Asset install did not report backup path' }
            $BackupPathOut.Value = ($backupLine -replace '^ASSET BACKUP:\s*', '').Trim()
        }
        'Restore' {
            $bp = $BackupPathOut.Value
            if (-not $bp) { throw 'Asset backup path missing from receipt' }
            & $tool -Action Restore -ManifestPath $AssetManifestPath -GameRoot $GameRoot -BackupPath $bp
        }
    }
}

function Invoke-RevoiceStep([string]$Step, [string]$Evidence) {
    if (-not $AllowLegacyRevoice) { throw 'legacy-revoice-v2 requires -AllowLegacyRevoice' }
    if (-not (Test-Path -LiteralPath $RevoiceCandidateDirectory)) { throw "Missing revoice candidate: $RevoiceCandidateDirectory" }
    $revoiceEvidence = Join-Path $Evidence 'legacy-revoice'
    $installer = Join-Path $PSScriptRoot 'install-revoice-candidate.ps1'
    $qa = @()
    if ($AllowPendingQa) { $qa += '-AllowPendingQa' }
    switch ($Step) {
        'Plan' { & $installer -Action Plan -CandidateDirectory $RevoiceCandidateDirectory -GameRoot $GameRoot -EvidenceDirectory $revoiceEvidence @qa }
        'Install' {
            if (-not $PSCmdlet.ShouldProcess($GameRoot, 'Install legacy-revoice-v2 (1886 WAVs)')) { return }
            & $installer -Action Install -CandidateDirectory $RevoiceCandidateDirectory -GameRoot $GameRoot -EvidenceDirectory $revoiceEvidence @qa
        }
        'Rollback' { & $installer -Action Rollback -GameRoot $GameRoot -EvidenceDirectory $revoiceEvidence }
    }
}

function Invoke-VerifyStep {
    $verifier = Join-Path $RepoRoot 'tools/audio/verify-polish-install-state.ps1'
    if (-not (Test-Path -LiteralPath $verifier)) { throw "Missing verifier: $verifier" }
    & $verifier -GameRoot $GameRoot -RepoRoot $RepoRoot
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) { throw 'Verify reported mismatches (see steam-verify-latest.json)' }
}

$spec = Get-ProfileSpec
foreach ($name in $Profile) {
    if (-not $spec.ContainsKey($name)) { throw "Unknown profile: $name" }
}

if ($Action -eq 'Plan') {
    Write-Plan
    if ($Profile -contains 'legacy-revoice-v2') {
        if (-not $AllowLegacyRevoice) { Write-Output 'NOTE: legacy-revoice-v2 listed but -AllowLegacyRevoice not set; Install would refuse.' }
        elseif (Test-Path -LiteralPath $RevoiceCandidateDirectory) {
            Write-Output 'legacy-revoice-v2 candidate directory exists (Plan-only dry listing above).'
        }
    }
    if ($Profile -contains 'story-xlingual-v1' -and $Action -eq 'Plan') {
        if (Test-Path -LiteralPath $StorySourceDirectory) {
            $wavCount = @(Get-ChildItem -LiteralPath $StorySourceDirectory -Filter '*.wav' -File).Count
            Write-Output "story source WAV count: $wavCount (expect 992)"
        }
        else { Write-Output "WARN: story source missing: $StorySourceDirectory" }
    }
    return
}

if ($Action -eq 'Verify') {
    if ($Profile -notcontains 'verify-promoted') {
        Write-Output 'Verify action runs verify-promoted checks regardless of profile list.'
    }
    Invoke-VerifyStep
    return
}

if (-not $EvidenceDirectory) { throw 'EvidenceDirectory is required for Install and Rollback' }
$evidence = Resolve-Evidence $EvidenceDirectory
Assert-GameClosed

$receiptPath = Join-Path $evidence 'polish-deploy-receipt.json'
$toolsRoot = Join-Path $PSScriptRoot '..'

if ($Action -eq 'Install') {
    New-Item -ItemType Directory -Path $evidence | Out-Null
    $steps = [System.Collections.Generic.List[object]]::new()
    $assetBackup = ''

    if ($Profile -contains 'assets-v4-first-area') {
        Invoke-AssetsStep 'Preflight' $evidence ([ref]$assetBackup)
        Invoke-AssetsStep 'Install' $evidence ([ref]$assetBackup)
        $steps.Add([ordered]@{ Profile = 'assets-v4-first-area'; AssetManifest = $AssetManifestPath; BackupPath = $assetBackup }) | Out-Null
    }

    if ($Profile -contains 'story-xlingual-v1') {
        Invoke-StoryStep 'Preflight' $evidence
        Invoke-StoryStep 'Install' $evidence
        $steps.Add([ordered]@{ Profile = 'story-xlingual-v1'; Evidence = (Join-Path $evidence 'story-xlingual'); Source = $StorySourceDirectory }) | Out-Null
    }

    if ($Profile -contains 'legacy-revoice-v2') {
        Invoke-RevoiceStep 'Install' $evidence
        $steps.Add([ordered]@{ Profile = 'legacy-revoice-v2'; Evidence = (Join-Path $evidence 'legacy-revoice'); Candidate = $RevoiceCandidateDirectory }) | Out-Null
    }

    if ($steps.Count -eq 0) { throw 'No installable profiles selected (first-area-l4-reference and verify-promoted are non-install)' }

    @{
        Kind      = 'SingularityVR polish deploy profile v1'
        CreatedAt = (Get-Date).ToString('o')
        GameRoot  = $GameRoot
        RepoRoot  = $RepoRoot
        Profiles  = @($Profile)
        Steps     = $steps
    } | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $receiptPath -Encoding UTF8

    Write-Output "RECEIPT: $receiptPath"
    Write-Output 'Run Verify after exit: deploy-polish-profile.ps1 -Action Verify -Profile verify-promoted'
    return
}

if ($Action -eq 'Rollback') {
    if (-not (Test-Path -LiteralPath $receiptPath)) { throw "Missing receipt: $receiptPath" }
    $receipt = Get-Content -LiteralPath $receiptPath -Raw | ConvertFrom-Json
    if ($receipt.Kind -cne 'SingularityVR polish deploy profile v1') { throw 'Wrong receipt kind' }
    if ($receipt.GameRoot -cne $GameRoot) { throw 'Receipt GameRoot mismatch' }

    $rollbackOrder = @('legacy-revoice-v2', 'story-xlingual-v1', 'assets-v4-first-area')
    foreach ($profileName in $rollbackOrder) {
        $step = @($receipt.Steps | Where-Object { $_.Profile -ceq $profileName }) | Select-Object -First 1
        if (-not $step) { continue }
        switch ($step.Profile) {
            'legacy-revoice-v2' { Invoke-RevoiceStep 'Rollback' $evidence }
            'story-xlingual-v1' { Invoke-StoryStep 'Rollback' $evidence }
            'assets-v4-first-area' {
                $backup = [string]$step.BackupPath
                $ref = [ref]$backup
                Invoke-AssetsStep 'Restore' $evidence $ref
            }
            default { throw "Unknown step profile: $($step.Profile)" }
        }
    }
    Write-Output 'ROLLBACK COMPLETE for orchestrated steps'
    return
}

throw "Unhandled Action: $Action"
