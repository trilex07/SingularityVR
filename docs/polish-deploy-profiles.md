# Polish deploy profiles (orchestrator)

MAIN deploys dubbing in **layers**. SIDE maintains one entry point that calls the
existing narrow installers and refuses the broad legacy script.

**Tool:** `tools/polish/deploy-polish-profile.ps1`  
**Do not use:** `tools/polish/install_polish.ps1`, `tools/run-recovery-trial.ps1`

## Profiles

| Profile | Purpose |
|---------|---------|
| `story-xlingual-v1` | 992 story WAVs from xlingual pack → `vr-assets/audio/polish/story` |
| `assets-v4-first-area` | 38 files: VI2 banks, PL manifests, `ui_GFx_fonts`, DDS, `texture_override.ini` |
| `legacy-revoice-v2` | 1886 WAV overlay (894 UI + 992 story); **off by default**; needs `-AllowLegacyRevoice` |
| `verify-promoted` | Read-only SHA vs `recovery-ready` manifests + story receipt |
| `first-area-l4-reference` | Plan-only description of PID37660 stack (not an auto bundle) |

Native UE3 bank **rebuild** (new xlingual → Vorbis inside `.xxx`) stays offline:
`tools/audio/prepare_native_story.py`, `native_story_package.py`. Not wired here.

## What made Polish NPC work (PID37660)

1. **Core lease** — DLL / injector / INI (and optional intro policy): `deploy-singularityvr.ps1`, separate from this orchestrator.
2. **`assets-v4-first-area`** — native first-area speech + font + manifests.
3. **Optional `legacy-revoice-v2`** — overlay WAVs; does **not** replace native gameplay lines.

992 xlingual alone does **not** equal full first-area native dubbing.

## MAIN examples (game closed)

```powershell
# Explain targets only
.\tools\polish\deploy-polish-profile.ps1 -Action Plan `
  -Profile story-xlingual-v1,assets-v4-first-area,first-area-l4-reference

# Promote story + 38-pack (fresh evidence dir under deployment/)
$ev = "deployment\hands-dubbing\deploy-$(Get-Date -Format 'yyyyMMdd-HHmmss')"
.\tools\polish\deploy-polish-profile.ps1 -Action Install `
  -Profile assets-v4-first-area,story-xlingual-v1 `
  -EvidenceDirectory $ev

# After install
.\tools\polish\deploy-polish-profile.ps1 -Action Verify -Profile verify-promoted

# Rollback using same evidence directory (receipt inside)
.\tools\polish\deploy-polish-profile.ps1 -Action Rollback `
  -Profile assets-v4-first-area,story-xlingual-v1 `
  -EvidenceDirectory $ev
```

## Guards and promotion

- Test **leases** restore baselines on exit; **promotion** keeps files and retains backups.
- Regenerate manifests when baselines change (xlingual receipt, 38-pack candidates, EA9EFFB1 runtime).
- Order for **Install:** assets → story → legacy revoice (if explicitly allowed).
- Order for **Rollback:** revoice → story → assets.

## Evidence

Install writes `polish-deploy-receipt.json` in `EvidenceDirectory` with backup paths
and sub-receipts (`story-xlingual/story-install.json`, asset backup under
`deployment/hands-dubbing/asset-backups/...`).

Verify output: `deployment/hands-dubbing/current-install-freeze-20261014/steam-verify-latest.json`

## SIDE / L1

```powershell
.\tools\tests\test-deploy-polish-profile.ps1
```
