# Centurion: refill without the native reload sequence

LATEST14:30 runtime/user result supersedes the old rejection below: source
56b3f16 lookup fix reaches ready1 and TWO conserved native transfers, HUD ACK,
user confirms reload works without old animation. First2 shots lack expected
gun firing animation: see `docs/centurion-post-refill-recoil.md` for the separate
presentation-hold candidate and its remaining physical gate. No full FX acceptance.

REOPENED after user session13:52: no animation BUT no ammo transfer, ready0
before invocation. Source6d7b201 is NOT physically accepted; do not promote its
L1/L2 receipt as a working backend. See `docs/centurion-refill-resolution.md` for
the bounded stage preflight needed before another full headset reload trial.

2026-10-17 SIDE, ZERO Steam. Existing physical polish is retained.

## Observation / hypothesis / falsifier BEFORE implementation

User accepts the cylinder/hand presentation but reports a firing delay and
camera shake after closing. Frozen session13:29 SHA256
8C668C978D92D2FFF4C5075E4F1052830E67DE5369A470B91A4DB7D5DFB1F4F1:
manual request10249093 -> conserved6/7 ACK10250656 -> ReloadFinished10251171.
This is1563ms to HUD refill and2078ms to the native completion callback.
Hiding the original hand animation did NOT cancel the native reload state.

Offline original registration proves a DIFFERENT no-parameter void native
`RvWeaponShared::RefillClip`, wrapper-derived slot, not ProcessInternal. Its
body obtains the inventory, calculates capacity minus clip, consults the two
ammo-mode predicates, removes reserve through inventory and adds the returned
quantity to clip. No state transition, animation or camera call is in that body.
The three native no-parameter int getters expose clip, bin and capacity.
The original AttemptReload instead dispatches ServerStartReload and enters
Reload. Reload.BeginState selects the pawn's combat animation node; its end
callback returns Active. Actual cause of the camera motion is NOT independently
traced: avoiding this whole sequence is the test, not a new global camera patch.

Hypothesis: a single permitted manual close can call ONLY audited RefillClip
on the natural engine AttemptReload thread, leaving the previous state intact.
Live getters must match the fresh identity-bound gesture snapshot before the
call, then show exactly min(capacity-clip,reserve) conserved transfer afterward.
No inventory writes, guessed UObject calls, timer edits or forced Active state.

Falsifiers: refill has no conserved transfer; ReloadFinished still follows this
operation; firing remains locked after HUD ACK/released trigger; camera still
shakes; repeated calls add ammo; NPCs/holster/other weapons change. A native call
counter and a build are NOT runtime acceptance. MAIN must capture these in one
short headset route without switching to chat.

## Intended experiment

`[Input] CenturionRefillOnly=0` (default OFF), requiring physical polish,
manual gesture controls, native tap and guard2. On one PERMIT only: resolve
original named wrappers through reflection, validate ABI and entire normalized
native bodies, read live counts, refill, re-read conservation. Unknown binding,
changed counts or failure rejects that permit; NEVER fall back to full native
reload after a possibly completed transfer. Other decisions/flag OFF preserve
the previous behavior. Input/HUD acknowledgement remains independent.

## Implemented and tested offline

L1_PASS:25 own-process reload CPU/ABI/D3D resource/GPU programs and56 Python
tests. The new CPU test exhausts clip0..6/reserve0..99, partial reserves, stale
snapshots, rejected capacities, double calls and faulty nonconserved transfer.
It also reads all four ORIGINAL archived native bodies and mutates every byte:
only the one cast rel32 is normalized; runtime additionally verifies the target
equals the inventory cast derived from AttemptReload. No absolute runtime VAs.
The x86 MinHook harness calls the same no-arg int/void thiscall types in its OWN
process: one permit calls refill WITHOUT original Attempt, preserves ECX/EAX/
stack, duplicates block, NPC forwards, experiment OFF preserves old dispatch.
Original cylinder-space, shell/state/Reset, palm, holster and gesture regressions
pass. These are NOT original engine execution, camera pixels or headset proof.

Private input hashes are in `docs/centurion-native-reload.md`. New independently
derived registration/vtable/fingerprint receipt:
`build/centurion-reload/refill-only/evidence.json`, SHA256
F65B3449963E9F2D4BF0702F19F5F277E9FB8C79F792CFE5E6646C71805F0E5F.
Tool: `tools/reload/inspect_refill_only.py`, outputs metadata only, not game code.
Reproduce tests with the previous test-centurion arguments plus
`-RefillEvidence build/centurion-reload/refill-only/evidence.json`.
Full log: `build/centurion-reload/refill-only/test.log`.

Snapshots freeze clip/reserve/capacity at the manual close, rather than letting
a later HUD poll silently change what that permit authorizes. The game-thread
operation rechecks live counts through audited original getters. This experiment
supports the measured finite six-round Centurion, not cheats/other capacities.
A postcondition fault latches rejection for the rest of the session. No retry,
inventory correction or full-reload fallback is attempted after such a fault.
Only the selected PERMIT branch changes. Previous stale/unknown-context
forwarding policy is retained and is NOT a promise to suppress every reload
under tracking loss. A later unsafe native fallback is a falsifier to investigate.

`centurion refill-only:` has its own32-record budget and logs ready, snapshot,
live before/after, time and fault. Results0=rejected binding/context/snapshot;
1=live counts changed;2=conserved native transfer;3=fault, no further transfers.
Only2 returns native bool true. Existing HUD ACK remains separate and may take
the next10Hz sample: do not call this a zero-frame input latency guarantee.

## MAIN integration / one short physical route

Integrate the new SOURCE commit onto CURRENT accepted MAIN, not this older SIDE
DLL over newer integration. Keep native gloves/knife, accepted cylinder/cases,
Polish/UI/assets and calibrated values. Rebuild x86/O2, verify deployment and
backup identities; change ONLY `[Input] CenturionRefillOnly=1` for this round
with the existing physical-polish/native-tap/guard2 dependencies already ON.
No deployment was performed by SIDE. Do not merge/promote before this gate.

User remains in PSVR2: fire3 shots, complete Circle/upward cases/left-shoulder/
insert/flick-close, then release and press fire promptly; repeat from empty.
Check no extra two-second lock and no reload camera jerk. Next, Circle and
store without completing reload, wait3s, draw: no free refill; pause/Options/
resume and normal exit. End early on discomfort. MAIN correlates conserved live
transfer AND HUD ACK, no ReloadFinished tail from that operation, no permits on
store or duplicate transfer. Check limited reserve if that save permits it.
Any wrong side, lost tracking, inventory change, camera jerk or full native tail
rejects the candidate. Keep the existing bounded restore lease and hashes.

## Clean L2 receipt

Source `6d7b201687c5d898fc2011ddf3b3ef87d0479911`, clean tree. L2_PASS
x86 PE32 (014C/010B), `/O2`, performance instrumentation OFF. Existing unrelated
compiler/analyzer warnings remain, not a warning-free build. DLL3204096 bytes:
`0EDD6DF07AA0F5F59A3E3904E557222BD3C11F5E8EF3D9E53BCEEF3F45C23B3A`.
Build receipt SHA256:
`6C118A95D04455D2625A433C24373341128DCCCF7E6FE13BE3B59B8DEF4725EA`.
Private candidate `build/centurion-reload/refill-candidate-6d7b201/` contains
DLL, build receipt and manifest, NO INI/assets. Build log:
`build/centurion-reload/refill-only/build.log`. This is not installed, accepted
or a restore baseline. Integrate ONLY source6d7b201 onto current MAIN that
already has the physically tested polish, then rebuild THERE.

L3/L4 UNKNOWN; physical removal of shake/fire delay is a justified experiment,
not a declaration of success. ZERO Steam launch/deploy/restore or installed-file
changes. Work remains on the existing PR48 branch, no merge.
