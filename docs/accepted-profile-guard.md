# Accepted-profile deployment guard

SIDE; no Steam writes or baseline promotion. Incident: the repository's partial
INI replaced a physically tuned installation, disabling Polish, hands and knife
and deleting per-weapon calibration. A newer DLL did not protect those settings.

Hypothesis: require a reviewed, pinned profile and exact change plan before
deployment. Compare ALL INI keys semantically, pin every listed dependency's
SHA-256, and check the actual selected source/target map. Reject missing keys,
unexpected additions, stale live state, unlisted writes and broad exceptions.
Falsifier: synthetic Polish/hands/knife regression, missing asset or stale
rollback can pass preflight or create installation files before rejection.

The guard does not infer acceptance from the current installation: it may
already be broken. MAIN must reconstruct a profile from verified receipts and
the user's accepted scope, review it, and only then set approved=true. Do not
snapshot today's regressed installation and call it accepted.

## Contract and coverage

`tools/deployment/AcceptedProfileGuard.ps1` is read-only when dot-sourced. Its
preflight rejects drift before backup creation, stopping a process or writing a
candidate. It checks the complete semantic INI (not just selected feature flags),
every inventoried file hash, exact candidate hashes and explicit INI changes.
Only `Render/AutoResX` and `Render/AutoResY` may be ignored, if listed. Comments,
ordering and whitespace are not acceptance evidence. Calibration, Polish,
hands, knife, HUD and diagnostics cannot disappear unnoticed from the profile.

Each write rechecks source, destination preimage and profile/plan hashes. Final
postflight verifies the complete protected state; a partial failure is an error,
never a success or an automatic acceptance. This is not an atomic multi-file
transaction or a security boundary against a concurrent privileged writer.
MAIN must keep exclusive deployment ownership and the game closed.

Integrated entry point: `tools/deploy-singularityvr.ps1`, Install with `-DllOnly`
or `-RuntimeDllPairOnly` (the latter optionally `-IncludeIni`), and file-only
Rollback. `-WhatIf` performs read-only preflight and returns before any backup,
process checks or writes. `Status` remains read-only without a profile.
Direct `spikes/view_matrix/build.ps1 -Install` is disabled before SDK/build I/O;
building normally remains unchanged.

**Intentional compatibility boundary:** broad texture/directory deployment and
opening-cutscene lease writes are blocked by this runner until adapted. Other
standalone scripts, notably `tools/steam-xrsim-singularity.ps1`, manual copies,
external installers and other worktrees are NOT guarded by this change. MAIN
must migrate their writes before claiming project-wide enforcement. Do not
merge into an active test window without reviewing that migration.

## Reviewed profile

Private, ignored default: `deployment/accepted-profile.json`. Required shape:

```json
{
  "schema": "singularityvr-accepted-profile-v1",
  "approved": true,
  "acceptedBy": "reviewer identity",
  "acceptanceScope": "specific physically accepted features and receipt",
  "iniPath": "Binaries/SingularityVR.ini",
  "ini": { "Section/Key": "value; include EVERY real key" },
  "ignoredIniKeys": ["Render/AutoResX", "Render/AutoResY"],
  "files": [{ "path": "Binaries/d3d9.dll", "sha256": "EXACT_64_HEX_HASH" }]
}
```

Example is a schema sketch, not a usable baseline. To generate an UNAPPROVED
draft, MAIN can dot-source the module and call `New-AcceptedProfileDraft
-GameRoot <explicit-root> -RelativeFiles <explicit-inventory>`. It only returns
an object; it never writes or approves it. Review independently, then save a
private JSON and set approval fields deliberately. Never commit game assets.

Inventory must include accepted runtime DLLs, UI/font packages, native speech
banks, localization manifests, relevant WAVs, hand/knife assets and anything
else intended to stay frozen. Enabled Polish/hand/knife features require a
small minimum dependency set in code, but **a manifest hash does not prove its
audio files exist**: enumerate and pin those too. No recursive completeness or
physical visual/audio acceptance is inferred. A null hash protects known
absence, so later additions must be deliberate.

## Exact candidate plan

Private `deployment/change-plan*.json`:

```json
{
  "schema": "singularityvr-change-plan-v1",
  "profileSha256": "EXACT_HASH_OF_REVIEWED_PROFILE_JSON",
  "reason": "camera-only experiment",
  "files": [{
    "path": "Binaries/d3d9.dll",
    "before": "EXACT_ACCEPTED_HASH", "after": "EXACT_CANDIDATE_HASH",
    "reason": "reviewed build and provenance"
  }],
  "ini": [{
    "key": "Render/VrCameraShakeSuppression",
    "before": "0", "after": "1", "reason": "single controlled variable"
  }]
}
```

Hashes above are placeholders; invalid until replaced. INI values are strings,
null means missing/deleted. File paths must already be inventoried (possibly
with null for absent). No wildcard allowances or unused allowances. Plans are
reviewed local artifacts, not authentication or proof of human approval.

MAIN first runs the existing deploy runner with `-WhatIf -DllOnly
-ArtifactDir <candidate> -AcceptedProfilePath <profile> -ChangePlanPath <plan>`.
Only after review does MAIN omit `-WhatIf`. To replace the INI, use
`-RuntimeDllPairOnly -IncludeIni` with the exact DLL pair and INI staged; ALL
unplanned keys must stay unchanged. A rejected partial/default INI must be
rebuilt from the reviewed accepted configuration, never allowed wholesale.

Rollback uses the SAME profile and plan plus the original `-BackupPath`.
It accepts only the expected candidate state and restores accepted hashes and
INI semantics. A newer/promoted DLL blocks stale rollback. There is no timer,
background daemon, silent restore or automatic baseline promotion. If the game
legitimately changes a protected setting during a test, review that difference;
do not add a broad exception just to force rollback through.

## Offline verification

Run `tools/tests/test-accepted-profile-guard.ps1`. All writes are generated fake
fixtures under ignored `build/accepted-profile-guard/<GUID>` in this checkout.
It checks feature loss, calibration loss, diagnostics drift, missing/tampered
assets, duplicate keys, invalid/stale plans, path escape, source/target races,
unapproved drafts, exact synthetic install/rollback, and stale rollback denial.
The actual runner is exercised only with `-WhatIf` on a synthetic Steam tree;
the synthetic PE header cannot execute. Legacy build `-Install` must reject.

No Steam/game launch, installed-file mutation, accepted baseline promotion or
headset acceptance belongs to these tests. Runtime features are unchanged.

Receipt 2026-10-16: L1_PASS, 37 rejection cases plus positive install/rollback
and actual-runner WhatIf checks, independently on Windows PowerShell 5.1 and
PowerShell 7. L2_PASS regression build at clean `4e7828e07f8d8f3ede68d5f9133c361effc068b0`,
x86 PE32 `/O2`, DLL SHA-256
`3B83750AC8C5BCABFA4C1160791BB03055C8D6F84ACAF288B8715ABC2A67F1D1`.
Build only, not staged/deployed; L3/L4 UNKNOWN. No real accepted profile exists
in this branch. MAIN's reviewed baseline/migration is required before activation.
