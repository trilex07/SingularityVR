# Import into Codex: SingularityVR / Centurion reload

Snapshot: 2026-10-16. Continue implementation, not just a report. This file replaces the long chat; inspect current Git/source before trusting snapshot values.

## Role and safety

You are SIDE unless the human explicitly assigns MAIN. ZERO Steam launches, deploys, restores, installed-file writes or live probes. MAIN alone owns Steam/PSVR2. Never merge, reset, force-push or edit another agent's checkout. Concurrent chat: create your OWN worktree/branch from this feature tip; reuse this checkout only after its previous writer stops. Commit/push your own work. No headset acceptance inferred from tests.

## Location / delivery

- Repo: `C:\Users\Trilex\vr-mods\SingularityVR-centurion-reload`
- Branch: `feature/centurion-manual-reload`; verify its current HEAD before editing.
- Draft PR: https://github.com/trilex07/SingularityVR/pull/37
- Stacked base: `feature/joint-knife-grip-and-stills`, merged through `f0b96ad`, NOT current main. MAIN has newer work elsewhere: integrate onto CURRENT accepted MAIN and rebuild; never deploy this standalone older base over it.
- Start by reading `AGENTS.md`, latest block of `STATUS.md`, then `docs/centurion-manual-reload.md` as needed. Use local `vr-mod-workflow` and `singularityvr-agent-test-bridge`; hard RE also requires `singularityvr-main-engineering`.

## Goal / implemented vs unknown

User has a Centurion save only and has NOT tested this reload. Preserve left-hip knife / right-hip gun holster. Ammo pouch is front-centre belt. Other weapons unchanged.

Centurion has an original SIX-CHAMBER CYLINDER, not a box magazine. Allocation-free FSM: remove with left grip, pull 15 cm, release; take fresh cylinder at belt; carry to socket, release; wait for native ammo acknowledgement. Left-hand ownership/cancellation protects knife/TMD/support actions. HUD observations are identity-bound, fresh and read-only; no inventory writes. Ack requires clip gain == reserve loss.

Optional carried preview uses original cylinder + original albedo, only while Carrying; native cylinder stays on gun. Unlit, no glove occlusion/fitted grip/native material parity. Mount/scale needs headset calibration. Capture/outer-state-restore failure latches presentation OFF until Reset/XR init; input/FSM unchanged.

Native probe is bounded/read-only in EXISTING interpreter hook. IMPORTANT: `Hook_ProcessEvent` actually hooks ProcessInternal(FFrame&,Result), NOT ProcessEvent(UFunction*,Parms). NEVER call its trampoline as Reload. Probe player-weapon ServerStartReload/ReloadFinished/AttemptReload/IsReloading and player-pawn AnimNotify_RefillClip. Offline Reload notify at 1.47052908 s does NOT prove live ammo commit or ABI. Dedicated begin/commit/end reload remains UNKNOWN; auto-reload/native animation untouched.

## Current evidence / candidate

ALL default OFF under `[Input]`, restart required:
`CenturionManualReload`, `CenturionNativeReloadTap`, `CenturionReloadProbe`, `CenturionClipPreview`.
NativeTap is only an opt-in 120 ms shared XInput Use/reload X tap, NOT dedicated Reload; avoid interactables.

L1: 8 CPU executables +27 Python tests; 100000-frame presentation-fault test; knife regressions 6 CPU +14 Python PASS. L2: clean source `e7a0c8986b10859a6832847c0d0a77f74d76d7f3`, x86 PE32 `/O2`.
DLL SHA256: `23AB074B813499DE2338EAC625079C3F5B0A5A1AB97196F6299F0B95BDCAB77A`.
Private candidate and exact asset hashes: `build/centurion-reload/candidate/centurion-preview.manifest.json`. DLL/manifest +two original assets verified; NO INI staged. Private game assets are ignored, NOT in Git. L3/L4 UNKNOWN. PR body may lag; manifests/source receipts win.

## Continue

Next weapons: `weapon_reload_prototypes.h` adds CPU-only Valkyrie/Volk/Kasimov profiles, default OFF, no runtime caller. Original clips/Reload tracks measured; suite now 9 CPU +33 Python PASS. Read `docs/next-weapon-reload-prototypes.md`. Mesh 8297 is Volk, not catalog's Spikeshot. Centurion runtime/candidate unchanged.

1. Safe offline work: validate probes/log analysis, cancellation/state restoration and regression tests. `tools/reload/analyze_reload_log.py --log <archived-log>` emits correlation-only JSON; 13 synthetic tests, no runtime/DLL changes. Prove producer/ABI before changing engine behavior; no guessed offsets, shader constants, ammo writes or native cylinder suppression.
2. Tests: inspect then run `tools/reload/test-centurion.ps1`, `tools/knife/test-knife.ps1`. Build `spikes/view_matrix/build.ps1` WITHOUT `-Install`; commit source first for clean provenance. `tools/reload/stage-preview.ps1` stages ONLY private candidate, never Steam.
3. MAIN's FIRST runtime variable: Probe=1, other three OFF; existing save, reserve ammo, fire twice, ordinary reload away from interactables. Correlate player identities/callback timing with conserved ammo change. Then separate manual dry-run (NativeTap OFF), then preview, finally optional shared-X backend only after evidence.
4. User stays in PSVR2 for the full agreed sequence, no chat/alt-tab mid-test. MAIN verifies exit/backups/hashes and restores safe install after test/timeout. Do not request another unchanged baseline.
