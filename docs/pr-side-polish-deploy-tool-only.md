# PR draft: SIDE polish deploy orchestrator

**Base:** `main`  
**Head:** `side/polish-deploy-tool-only`  
**Compare:** https://github.com/trilex07/SingularityVR/compare/main...side/polish-deploy-tool-only

## Summary

Adds a MAIN-facing orchestrator for **layered** Polish dubbing deployment without touching `d3d9.dll`, `dinput8.dll`, or `SingularityVR.ini`. Composes existing narrow installers (`install-story-xlingual.ps1`, `test-prepared-assets.ps1`, optional `install-revoice-candidate.ps1`). Includes read-only Steam verify helper, investigation notes, docs, and an offline Plan test.

**Does not merge:** broad `install_polish.ps1` or stale `run-recovery-trial.ps1` flows.

## Files

- `tools/polish/deploy-polish-profile.ps1` — Plan / Install / Verify / Rollback + receipt
- `docs/polish-deploy-profiles.md` — profiles and PID37660 reference stack
- `tools/audio/verify-polish-install-state.ps1` — SHA check (38 assets + 992 story + core manifest)
- `tools/tests/test-deploy-polish-profile.ps1` — L1 parser + Plan
- `docs/SIDE-AUDIO-INVESTIGATION-20261014.md` — root-cause notes (lease vs promotion, native banks)

## Test plan

- [ ] `powershell -File tools/tests/test-deploy-polish-profile.ps1` → PASS
- [ ] `deploy-polish-profile.ps1 -Action Plan -Profile story-xlingual-v1,assets-v4-first-area` (no Steam writes)
- [ ] Confirm Install profile list does **not** include DLL/INI paths
- [ ] MAIN: after merge, use `-Action Verify` before/after real deploy (optional)

## Notes

- Native xlingual → UE3 bank rebuild remains `tools/audio/prepare_native_story.py` / `prepare_xlingual_banks.py` (separate PR/work).
- Recovery/TV/renderer changes live on other branches; this PR is **deploy tooling only**.

## gh command (after `gh auth login`)

```powershell
gh pr create --repo trilex07/SingularityVR --base main --head side/polish-deploy-tool-only --draft --title "tools(polish): deploy profile orchestrator for layered dubbing" --body-file docs/pr-side-polish-deploy-tool-only.md
```

(Strip this “gh command” section from the PR body if you prefer a shorter description.)
