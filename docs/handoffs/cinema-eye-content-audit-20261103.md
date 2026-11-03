# Cinema eye-content and field-bound audit

## State and failed hypotheses

Physical feedback on EBA33BED: left eye still shows screen/content further than right. Gun attachment accepted. Both common-FOV initial fit and zero-disparity frame translation did not resolve the reported cinema asymmetry. No further geometry changes in this diagnostic branch.

Actual current-session receipts prove equal source rect dimensions, quad sizes and unequal per-eye translated positions as intended. A read-only observer opened process 15020 with QUERY_LIMITED_INFORMATION | VM_READ only, after matching installed and local DLL SHA EBA33BED and linker map. Eight eye-pose samples show identical left/right quaternions. Thus canted/different eye orientations are disproven for those samples. Actual angle FOVs: left (-61.5,+43.5), right (-43.5,+61.5), vertical +/-53.0401 degrees. Hardware display frusta are asymmetric.

These samples were taken after headset rendering stopped (shouldRender=0), not the user's visible problematic frame. The unused CPU slots were zero. The actual zero-copy record was decoded with MSVC-confirmed sizeof(CopySlot)=160, pose offset52, FOV108, valid40, cineStereo20; it contains cineStereo=0. Do NOT infer a failing cinema-frame corner from either record. Private data: build/cinema-orientation-audit/; no extracted game assets committed.

## Specific missing evidence and one bounded diagnostic

Need ONE rendered stereo cinema frame with exact final quad transforms, image rectangles, eye poses/FOV, calculated corner bounds and actual SBS source pixels. This distinguishes:

1. Identical geometric border but one eye clips its asymmetric FOV.
2. Border in bounds for both eyes but the source image contains a different matte/content extent.
3. Correct source and geometric bounds but compositor/lens-specific presentation differs.

New default-OFF [Diagnostic] CinemaEyeAudit=1 waits for 30 eligible stereo-cinema frames with valid tracking and focused session, then logs one final pair, classifies all four corners and calls the existing read-only backbuffer capture once. It logs the matching xrEndFrame result. No new GPU resource/render route, matrix writes or capture in gameplay. Disabled flag gives no capture/readback. One capture may visibly hitch; no per-frame synchronization. Capture is PRE-D3D11/OLED/compositor, explicitly not HMD pixels. Capture failure/cooldown is visible through existing capture receipts and cannot be claimed success.

The audit does not fix the geometry; it deliberately preserves EBA33BED to avoid another speculative candidate. If final edges lie inside both frusta, investigate source matte/postprocess content using the captured SBS. If both source and geometry agree, obtain compositor evidence before any further transforms. No camera/weapon/TMD changes.

## Verification and next gate

- New optimized x86/x64 fixture uses actual measured asymmetric PSVR2 FOV, reproduces one-eye clipping with equal edge rays; tests central in-bounds case, behind/invalid rejection, stable one-shot and OFF gates.
- Run gaze/cinema real D3D9Ex pixel/state regressions and x86 /O2 build. Preserve native ABI.
- Physical/Steam capture PENDING. Active installation/session untouched until game closed and scoped diagnostic INI change reviewed.
- One test only: enter the same cinema while wearing PSVR2, look at its centre for a second, then exit normally. No shooting, extra scenes or repeated baseline tests necessary. Once a successful receipt and capture exist, diagnose offline before requesting another headset session.

## Built diagnostic identity

Source: 3da12f8c495b26516a22c073c087e123aa225aec.
DLL SHA256: 59171C5884107BC71C732EF48C96DD32BCE44805C9FBF0CD1847D6C1BB1E5786.
Optimized x86 PE32 build PASS; actual compiled factory literal [eax+20Ch] and FName stack regression PASS. Audit/cinema geometry fixtures x86/x64 PASS. D3D9Ex actual pixel/state/caption/corner fixtures PASS; legacy D3D9 HAL NOT RUN (host rejected device 8876086C). No physical visual PASS claimed.

Private immutable artifacts, linker map, deployment profile and exact two-entry change plan are staged under build/cinema-orientation-audit/. Read-only accepted-profile preflight PASS for all protected installed files and full INI. Only DLL and Diagnostic/CinemaEyeAudit=1 are planned; no assets or other settings change. Steam deployment NOT RUN: active Singularity process blocks writes. Installed EBA33BED remains unchanged.

## Controlled MAIN deployment

2026-11-03 11:30 CET: user confirmed normal game exit. Elevated process inventory found no active game or restore/deploy writer. Project deploy tool installed only the staged DLL and INI; backup deployment/install-backups/20261103-113046, report deployment/install-reports/20261103-113047.json. Installed DLL matches 59171C58 full hash above; installed INI SHA256 A9F4C8932E2C49DBC524B7612C43A4F03A914B00F3900C925A320F6D8202BEA7. Full accepted-profile Post guard PASS (all protected files plus parsed INI). No launch, timer or automatic restore started. Runtime capture remains PENDING.
